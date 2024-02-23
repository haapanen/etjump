/*
 * MIT License
 *
 * Copyright (c) 2024 ETJump team <zero@etjump.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "etj_user_repository.h"

#include <utility>

#include "etj_log.h"

namespace {
ETJump::Log logger("user repository");
}

void ETJump::UserRepository::initialize() { migrate(); }

void ETJump::UserRepository::migrate() {
  _database->addMigration(
      "initial",
      {
          // clang-format off
      R"(
        create table user (
          id integer primary key autoincrement,
          name text not null,
          level integer not null,
          last_seen timestamp not null,
          title text null,
          commands text null,
          greeting text null
        );
      )",
      R"(
        create table user_name (
          name text not null,
          clean_name text not null,
          user_id integer not null,
          primary key (user_id, name),
          foreign key (user_id) references user(id)
        );
      )",
      R"(
        create table user_guid (
          guid text primary key,
          user_id integer not null,
          foreign key (user_id) references user(id)
        );
      )",
      R"(
        create table user_hardware_id (
          hardware_id text not null,
          user_id integer not null,
          foreign key (user_id) references user(id)
        );
      )",
      "create index idx_user_guid_user_id on user_guid(user_id);",
      "create index idx_user_hardware_id_user_id on user_hardware_id(user_id);",
    }
  );
  // clang-format on

  _database->applyMigrations();

  int count = 0;
  _database->sql << "select count(*) from user;" >> count;

  if (count == 0) {
    tryToMigrateRecords();
  }
}

void ETJump::UserRepository::shutdown() {}

void ETJump::UserRepository::insertUser(const ETJump::User &user) {
  auto bindUserFields = [](sqlite::database_binder &binder,
                           const ETJump::User &user) {
    binder << user.name << user.level << user.lastSeen.toDateTimeString()
           << user.title << user.commands << user.greeting;
  };

  auto userId = 0;

  if (user.id != 0) {
    auto binder = _database->sql << R"(
      insert into user (
        id,
        name,
        level,
        last_seen,
        title,
        commands,
        greeting
      ) values (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
      )
      returning id;
    )";

    binder << user.id;

    bindUserFields(binder, user);

    binder >> userId;
  } else {
    auto binder = _database->sql << R"(
      insert into user (
        name,
        level,
        last_seen,
        title,
        commands,
        greeting
      ) values (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
      )
      returning id;
    )";

    bindUserFields(binder, user);

    binder >> userId;
  }

  for (const auto &guid : user.guids) {
    _database->sql << R"(
      insert into user_guid (
        guid,
        user_id
      ) values (?, ?);
    )" << guid << userId;
  }

  for (const auto &hwid : user.hardwareIds) {
    _database->sql << R"(
      insert into user_hardware_id (
        hardware_id,
        user_id
      ) values (?, ?);
    )" << hwid << userId;
  }

  for (const auto &name : user.names) {
    _database->sql << R"(
      insert into user_name (
        name,
        clean_name,
        user_id
      ) values (?, ?, ?);
    )" << name.text << name.cleanText
                   << user.id;
  }
}

void ETJump::UserRepository::updateUser(const ETJump::User &user,
                                        int changedFields) {}

void ETJump::UserRepository::deleteUser(int userId) {}

void ETJump::UserRepository::tryToMigrateRecords() {
  std::vector<User> oldUsers;

  User user{};

  _oldDatabase->sql << R"(
    select
      distinct
      users.id,
      users.guid,
      users.level,
      users.lastSeen,
      users.name,
      users.hwid,
      users.title,
      users.commands,
      users.greeting,
      name.clean_name,
      name.name as previous_name
    from users
    left join name on name.user_id=users.id
    order by users.id;
  )" >>
      [&oldUsers, &user](int id, std::string guid, int level, int lastSeen,
                         std::string name, std::string hwid, std::string title,
                         std::string commands, std::string greeting,
                         std::string previousCleanName,
                         std::string previousName) {
        if (user.id != id) {
          // if this is the first processed user, we do not yet want to
          // push it to the vec as it's not yet fully processed
          if (user.id != 0) {
            oldUsers.push_back(user);
          }

          auto hwids = Container::filter(
              StringUtil::split(hwid, ","),
              [](const auto &item) { return trim(item).length() > 0; });

          user = User{id,
                      std::move(name),
                      level,
                      Time::fromInt(lastSeen),
                      std::move(title),
                      std::move(commands),
                      std::move(greeting),
                      hwids,
                      {guid},
                      {{previousName, previousCleanName}}};
        } else {
          user.names.emplace_back(previousName, previousCleanName);
        }
      };

  if (user.id != 0) {
    oldUsers.push_back(user);
  }

  _database->sql << "begin;";

  for (const auto &u : oldUsers) {
    try {
      insertUser(u);
    } catch (std::runtime_error &e) {
      logger.error("failed to insert user %d: %s", u.id, e.what());
      Log::processMessages();
    }
  }

  _database->sql << "commit;";
}
