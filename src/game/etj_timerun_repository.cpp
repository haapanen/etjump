#include "etj_timerun_repository.h"

#include "etj_container_utilities.h"
#include "etj_database_v2.h"

void ETJump::TimerunRepository::initialize() { migrate(); }

void ETJump::TimerunRepository::shutdown() { _database = nullptr; }

std::vector<ETJump::Timerun::Season> ETJump::TimerunRepository::
getActiveSeasons(const Time &currentTime) const {
  std::vector<Timerun::Season> activeSeasons;

  auto currentTimeStr = currentTime.toDateTimeString();

  _database->sql << "select id, name, start_time, end_time from season where "
      "start_time <= ? and (end_time is null or end_time > ?);"
      << currentTimeStr << currentTimeStr >>
      [this, &activeSeasons](int id, std::string name, std::string startTimeStr,
                             std::string endTimeStr) {
        auto startTime = Time::fromString(startTimeStr);
        opt<Time> endTime;
        if (endTimeStr.length() != 0) {
          endTime = opt<Time>(Time::fromString(endTimeStr));
        }

        activeSeasons.push_back(Timerun::Season{id, name, startTime, endTime});
      };

  return activeSeasons;
}

ETJump::Timerun::Record getRecordFromStandardQueryResult(
    int seasonId, std::string map, std::string runName, int userId, int time,
    std::string checkpointsString, std::string recordDate,
    std::string playerName, std::string metadataString) {
  auto checkpoints = ETJump::Container::map(
      ETJump::Container::filter(
          ETJump::StringUtil::split(checkpointsString, ","),
          [](const std::string &input) {
            return ETJump::trim(input).length() > 0;
          }),
      [](const std::string &checkpoint) {
        try {
          return std::stoi(ETJump::trim(checkpoint));
        } catch (const std::runtime_error &e) {
          // TODO: magic const
          return -1;
        }
      });
  ETJump::Time recordDateTime{};
  try {
    recordDateTime = ETJump::Time::fromString(recordDate);
  } catch (const std::runtime_error &e) {
    recordDateTime = ETJump::Time::fromString("1900-01-01 00:00:00");
  }

  std::map<std::string, std::string> metadata;
  for (const auto &kvp :
       ETJump::Container::map(
           ETJump::StringUtil::split(metadataString, ","),
           [](const std::string &kvp) {
             return ETJump::StringUtil::split(kvp, "=");
           })) {
    if (kvp.size() != 2) {
      continue;
    }

    metadata[kvp[0]] = kvp[1];
  }

  ETJump::Timerun::Record record;
  record.seasonId = seasonId;
  record.map = map;
  record.run = runName;
  record.userId = userId;
  record.time = time;
  record.recordDate = recordDateTime;
  record.checkpoints = checkpoints;
  record.playerName = playerName;
  record.metadata = metadata;

  return record;
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsForPlayer(
    const std::vector<int> activeSeasons, const std::string &map, int userId) {
  auto parameters = StringUtil::join(
      Container::map(activeSeasons,
                     [](int season) { return std::to_string(season); }),
      ", ");

  std::vector<Timerun::Record> runs;

  // Above parameters are just season IDs, no risk of SQL injection
  // hence the direct interpolation
  _database->sql << stringFormat(R"(
          select
            season_id,
            map,
            run,
            user_id,
            time,
            checkpoints,
            record_date,
            player_name,
            metadata
          from record
          where season_id in (%s) and
            map=? and
            user_id=?;
        )",
                                 parameters)
      << map << userId >>
      [&runs, this](int seasonId, std::string map, std::string runName,
                    int userId, int time, std::string checkpointsString,
                    std::string recordDate, std::string playerName,
                    std::string metadataString) {

        runs.push_back(std::move(getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString)));
      };

  return runs;
}

ETJump::Timerun::Season
ETJump::TimerunRepository::addSeason(Timerun::AddSeasonParams params) {
  int count = 0;
  _database->sql << R"(
                      select count(name) from season where name=? collate nocase;
                    )"
      << params.name >>
      count;

  if (count > 0) {
    throw std::runtime_error(
        stringFormat("Cannot add season `%s` as it already exists.",
                     params.name));
  }

  if (params.endTime.hasValue()) {
    if (params.startTime >= params.endTime.value()) {
      throw std::runtime_error("Start time cannot be after end time");
    }
  }

  std::string insert = R"(
                    insert into season (
                      name,
                      start_time,
                      end_time
                    ) values (
                      ?,
                      ?,
                      ?
                    );
                  )";

  if (params.endTime.hasValue())
    _database->sql << insert << params.name
        << params.startTime.toDateTimeString()
        << (*params.endTime).toDateTimeString();
  else
    _database->sql << insert << params.name
        << params.startTime.toDateTimeString() << nullptr;

  return Timerun::Season{static_cast<int>(_database->sql.last_insert_rowid()),
                         params.name, params.startTime, params.endTime};
}

std::vector<ETJump::Timerun::Record> ETJump::TimerunRepository::
getRecordsForPlayer(const std::vector<int> &activeSeasons,
                    const std::string &map, const std::string &run,
                    int userId) {
  auto records = std::vector<Timerun::Record>();

  _database->sql << stringFormat(R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where season_id in (%s) and
      map=? and
      run=? and
      user_id=?;
    )",
                                 StringUtil::join(activeSeasons, ", "))
      << map << run << userId >>
      [&records](int seasonId, std::string map, std::string runName, int userId,
                 int time, std::string checkpointsString,
                 std::string recordDate,
                 std::string playerName, std::string metadataString) {
        auto record = getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString);

        records.push_back(record);
      };

  return records;
}

std::vector<ETJump::Timerun::Record>
ETJump::TimerunRepository::getRecordsForRun(
    const std::string &map, const std::string &run) const {
  throw std::runtime_error("Not implemented");
}

void ETJump::TimerunRepository::insertRecord(const Timerun::Record &record) {
  _database->sql << R"(
    insert into record (
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    ) values (
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?,
      ?
    );
  )" << record.seasonId << record.map << record.run << record.userId << record.
      time << StringUtil::join(record.checkpoints, ",") << record.recordDate.
      toDateTimeString() << record.playerName << serializeMetadata(
          record.metadata);
}

void ETJump::TimerunRepository::updateRecord(const Timerun::Record &record) {
  _database->sql << R"(
    update
      record
    set
      time=?,
      checkpoints=?,
      record_date=?,
      player_name=?,
      metadata=?
    where
      season_id=? and
      map=? and
      run=? and
      user_id=?;
  )" << record.time
      << StringUtil::join(record.checkpoints, ",") << record.recordDate.
      toDateTimeString() << record.playerName << serializeMetadata(
          record.metadata) << record.seasonId << record.map << record.run <<
      record.userId;
}

ETJump::opt<ETJump::Timerun::Record>
ETJump::TimerunRepository::getTopRecord(int seasonId,
                                        const std::string &map,
                                        const std::string &run) {
  opt<Timerun::Record> record;
  _database->sql << R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where 
      season_id=? and
      map=? and
      run=?
    order by time asc
    limit 1
    )"
      << seasonId << map << run >>
      [&record](int seasonId, std::string map, std::string runName, int userId,
                int time, std::string checkpointsString,
                std::string recordDate, std::string playerName,
                std::string metadataString) {
        record = opt<Timerun::Record>(getRecordFromStandardQueryResult(
            seasonId, map, runName, userId, time, checkpointsString, recordDate,
            playerName, metadataString));
      };

  return record;
}

void ETJump::TimerunRepository::editSeason(
    const Timerun::EditSeasonParams &params) {
  int seasonId = -1;
  Time startTime;
  ETJump::opt<Time> endTime;

  _database->sql << R"(
    select
      id,
      start_time,
      end_time
    from season
    where name=?
    collate nocase
  )" << params.name >>
      [&](int sid, std::string s, std::unique_ptr<std::string> e) {
        seasonId = sid;
        startTime = Time::fromString(s);
        if (e) {
          endTime = ETJump::opt<ETJump::Time>(Time::fromString(*e));
        }
      };

  if (seasonId < 0) {
    throw std::runtime_error(
        stringFormat("No season matching name `%s`", params.name));
  }

  std::vector<std::string> updatedFields;
  std::vector<std::string> updatedParams;
  bool anythingToUpdate = false;

  Time newStartTime = startTime;
  opt<Time> newEndTime = endTime;

  if (params.startTime.hasValue()) {
    newStartTime = params.startTime.value();
    anythingToUpdate = true;
    updatedFields.push_back("start_time");
    updatedParams.push_back(params.startTime.value().toDateTimeString());

  } else if (params.endTime.hasValue()) {
    newEndTime = params.endTime;
    anythingToUpdate = true;
    updatedFields.push_back("end_time");
    updatedParams.push_back(params.endTime.value().toDateTimeString());
  }

  if (newEndTime.hasValue() && newEndTime.value() < newStartTime) {
    throw std::runtime_error("End time cannot be before start time.");
  }

  if (!anythingToUpdate) {
    return;
  }

  std::string updatedFieldsString =
      StringUtil::join(Container::map(updatedFields, [](auto a) {
        return a + "=?";
      }), ",");

  auto query = stringFormat(R"(
    update
      season
    set
      %s
    where
      id=?
  )", updatedFieldsString);

  auto q = _database->sql << query;

  for (const auto &p : updatedParams) {
    q << p;
  }
  q << seasonId;
}

std::vector<ETJump::Timerun::Record> ETJump::TimerunRepository::getRecords(
    const Timerun::PrintRecordsParams &params) {
  if (!params.map.hasValue()) {
    throw std::runtime_error("Map has to be defined for getRecords query");
  }

  auto season = params.season.hasValue() ? params.season.value() : "Default";
  auto map = params.map.value();
  auto runSpecified = params.run.hasValue();

  auto seasons = getSeasonsForName(season, false);

  if (seasons.size() == 0) {
    throw std::runtime_error(
        stringFormat("No season matches name `%s`", season));
  }

  std::string seasonPlaceholders =
      StringUtil::join(Container::map(seasons, [](const auto &s) {
    return "season_id=?"; }), " or ");

  std::string runPlaceholder = runSpecified ? "and run=?" : "";

  auto query = stringFormat(R"(
    select
      season_id,
      map,
      run,
      user_id,
      time,
      checkpoints,
      record_date,
      player_name,
      metadata
    from record
    where 
      (%s) and
      map like ?
      %s
    order by season_id, map, run, time asc
  )",
                            seasonPlaceholders, runPlaceholder);

  auto binder = _database->sql << query;

  for (const auto & s : seasons) {
    binder << s.id;
  }

  binder << map;

  if (runSpecified) {
    binder << params.run.value();
  }

  std::vector<Timerun::Record> records;

  binder >> [&records](int seasonId, std::string map, std::string runName,
                       int userId, int time, std::string checkpointsString,
                       std::string recordDate, std::string playerName,
                       std::string metadataString) {
    auto record = getRecordFromStandardQueryResult(
        seasonId, map, runName, userId, time, checkpointsString, recordDate,
        playerName, metadataString);

    records.push_back(record);
  };

  return records;
}

std::vector<ETJump::Timerun::Season>
ETJump::TimerunRepository::getSeasonsForName(
    const std::string &name, bool exact) {
  std::string query;

  std::vector<Timerun::Season> seasons;

  auto handler = [&seasons](int id, const std::string &name,
                            const std::string &startTime,
                            std::unique_ptr<std::string> endTime) {
    seasons.push_back(Timerun::Season{id, name, Time::fromString(startTime),
                                      (endTime
                                         ? opt<Time>(Time::fromString(*endTime))
                                         : opt<Time>())});
  };

  if (exact) {
    query = R"(
      select
        id,
        name,
        start_time,
        end_time
      from season
      where name=?
      collate nocase
    )";

    _database->sql << query << name >> handler;
  } else {
    query = R"(
      select
        id,
        name,
        start_time,
        end_time
      from season
      where name like ?
      collate nocase
    )";

    _database->sql << query << "%" + name + "%" >> handler;
  }

  return seasons;
}

void ETJump::TimerunRepository::tryToMigrateRecords() {
  int count = 0;
  _oldDatabase->sql << "select count(*) from sqlite_master where tbl_name='records'" >> count;
  if (count == 0) {
    return;
  }

  std::vector<Timerun::Record> oldRecords;

  _oldDatabase->sql << R"(
    select
        id,
        time,
        record_date,
        map,
        run,
        user_id,
        player_name
    from records;
  )" >>
      [&oldRecords](int id, int time, int recordDate, std::string map,
                    std::string run, int userId, std::string playerName) {
        Timerun::Record r{};
        r.seasonId = 1;
        r.map = map;
        r.run = run;
        r.time = time;
        r.recordDate = Time::fromInt(recordDate);
        r.userId = userId;
        r.playerName = playerName;
        r.checkpoints = std::vector<int>(16, -1);
        r.metadata = {{"mod_version", "unknown(imported)"}};
        oldRecords.push_back(r);
      };

  _database->sql << "begin;";

  for (const auto & r : oldRecords) {
    insertRecord(r);
  }

  _database->sql << "commit;";
}

void ETJump::TimerunRepository::migrate() {
  _database->addMigration(
      "initial",
      {R"(
          create table season (
              id integer primary key autoincrement,
              name text not null,
              start_time timestamp not null,
              end_time timestamp null
          );
        )",
       R"(
            insert into season (
              id,
              name,
              start_time,
              end_time
            ) values (
              1,
              'Default',
              current_timestamp,
              null
            );
          )",
       R"(
            create table record (
              season_id integer not null,
              map text not null,
              run text not null,
              user_id int not null,
              time int not null,
              checkpoints text not null,
              record_date timestamp not null,
              player_name text not null,
              metadata text not null default '',
              primary key (season_id, map, run, user_id),
              foreign key (season_id) references season(id)
            );
          )",
       "create index idx_season_id_map on record(season_id, map);",
       "create index idx_season_id_map_run on record(season_id, map, run);",
       "create index idx_season_id_map_run_user_id on record(season_id, map, "
       "run, user_id);"});

  _database->applyMigrations();

  int count = 0;
  _database->sql << "select count(*) from record" >> count;

  if (count == 0) {
    tryToMigrateRecords();
  }
}

std::string ETJump::TimerunRepository::serializeMetadata(
    std::map<std::string, std::string> metadata) {
  std::string result;
  for (const auto &kvp : metadata) {
    // NOTE: we're not escaping = so if payload contains = this will fail...
    result += kvp.first + "=" + kvp.second;
  }
  return result;
}