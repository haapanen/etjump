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

#pragma once
#include <memory>

#include "etj_database_v2.h"
#include "etj_user_models.h"

namespace ETJump {
class UserRepository {
public:
  explicit UserRepository(std::unique_ptr<ETJump::DatabaseV2> database,
                          std::unique_ptr<ETJump::DatabaseV2> oldDatabase)
    : _database(std::move(database)), _oldDatabase(std::move(oldDatabase)) {
  }

  void initialize();
  void migrate();
  void shutdown();

  enum class ChangedField : int {
    Name = 1,
  };

  void insertUser(const ETJump::User &user);
  void updateUser(const ETJump::User &user, int changedFields);
  void deleteUser(int userId);

private:
  void tryToMigrateRecords();

  std::unique_ptr<ETJump::DatabaseV2> _database;
  std::unique_ptr<ETJump::DatabaseV2> _oldDatabase;
};
}
