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

#include <stdexcept>

#include "etj_utilities.h"
#include "etj_save_system.h"
#include "etj_map_statistics.h"

std::vector<int> Utilities::getSpectators(int clientNum) {
  std::vector<int> spectators;

  for (auto i = 0; i < level.numConnectedClients; i++) {
    gentity_t *player = g_entities + level.sortedClients[i];

    if (level.sortedClients[i] == clientNum) {
      continue;
    }

    if (!player->client) {
      continue;
    }

    if (player->client->sess.sessionTeam != TEAM_SPECTATOR) {
      continue;
    }

    if (player->client->ps.clientNum == clientNum) {
      spectators.push_back(g_entities - player);
    }
  }

  return spectators;
}

static void SelectCorrectWeapon(gclient_t *cl) {
  auto primary = cl->sess.playerWeapon;
  auto secondary = cl->sess.playerWeapon2;

  // early out if our currently held weapon is allowed
  // even though disallowed weapons are already removed, we'll still get to
  // keep any if it's currently equipped, so we force a weapon swap later
  if (!BG_WeaponDisallowedInTimeruns(cl->ps.weapon)) {
    return;
  }

  // our currently held weapon was removed, so swap to a valid one
  if (COM_BitCheck(cl->ps.weapons, primary) &&
      BG_WeaponHasAmmo(&cl->ps, primary)) {
    cl->ps.weapon = primary;
  } else if (COM_BitCheck(cl->ps.weapons, secondary) &&
             BG_WeaponHasAmmo(&cl->ps, secondary)) {
    cl->ps.weapon = secondary;
  } else {
    cl->ps.weapon = WP_KNIFE;
  }
}

static void RemovePlayerProjectiles(int clientNum) {
  // Iterate entitylist and remove all projectiles
  // that belong to the activator
  for (int i = MAX_CLIENTS; i < level.num_entities; i++) {
    auto ent = g_entities + i;
    if (ent->s.eType == ET_MISSILE) {
      if (ent->parent && ent->parent->s.number == clientNum) {
        G_FreeEntity(ent);
      }
    }
  }
}

void Utilities::startRun(int clientNum) {
  gentity_t *player = g_entities + clientNum;

  player->client->sess.timerunActive = qtrue;
  ETJump::UpdateClientConfigString(*player);

  if (!(player->client->sess.runSpawnflags &
        static_cast<int>(ETJump::TimerunSpawnflags::NoSave))) {
    ETJump::saveSystem->clearTimerunSaves(player);
  }

  // If we are debugging, just exit here
  if (g_debugTimeruns.integer > 0) {
    return;
  }

  RemovePlayerWeapons(clientNum);
  RemovePlayerProjectiles(clientNum);
  SelectCorrectWeapon(player->client);
  ClearPortals(player);
}

void Utilities::stopRun(int clientNum) {
  gentity_t *player = g_entities + clientNum;

  player->client->sess.timerunActive = qfalse;
  ETJump::UpdateClientConfigString(*player);
}

namespace UtilityHelperFunctions {
static void FS_ReplaceSeparators(char *path) {
  char *s;

  for (s = path; *s; s++) {
    if (*s == '/' || *s == '\\') {
      *s = PATH_SEP;
    }
  }
}

static char *BuildOSPath(const char *file) {
  char base[MAX_CVAR_VALUE_STRING] = "\0";
  char temp[MAX_OSPATH] = "\0";
  char game[MAX_CVAR_VALUE_STRING] = "\0";
  static char ospath[2][MAX_OSPATH] = {"\0", "\0"};
  static int toggle;

  toggle ^= 1; // flip-flop to allow two returns without clash

  trap_Cvar_VariableStringBuffer("fs_game", game, sizeof(game));
  trap_Cvar_VariableStringBuffer("fs_homepath", base, sizeof(base));

  Com_sprintf(temp, sizeof(temp), "/%s/%s", game, file);
  FS_ReplaceSeparators(temp);
  Com_sprintf(ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp);

  return ospath[toggle];
}
} // namespace UtilityHelperFunctions

void Utilities::RemovePlayerWeapons(int clientNum) {
  auto *cl = (g_entities + clientNum)->client;

  for (int i = 0; i < WP_NUM_WEAPONS; i++) {
    if (BG_WeaponDisallowedInTimeruns(i)) {
      COM_BitClear(cl->ps.weapons, i);
    }
  }

  cl->ps.grenadeTimeLeft = 0;
}

bool Utilities::inNoNoclipArea(gentity_t *ent) {
  trace_t trace;
  trap_TraceCapsule(&trace, ent->client->ps.origin, ent->client->ps.mins,
                    ent->client->ps.maxs, ent->client->ps.origin,
                    ent->client->ps.clientNum, CONTENTS_NONOCLIP);

  // if we're touching a no-noclip area, do another trace for solids
  // so that we don't get instantly stuck in a wall, unable to noclip
  // if we fly to a no-noclip area through a wall
  if (level.noNoclip) {
    if (trace.fraction == 1.0f) {
      trap_TraceCapsule(&trace, ent->client->ps.origin, ent->client->ps.mins,
                        ent->client->ps.maxs, ent->client->ps.origin,
                        ent->client->ps.clientNum, CONTENTS_SOLID);

      if (!trace.allsolid) {
        return true;
      }
    }
  } else {
    if (trace.fraction != 1.0f) {
      trap_TraceCapsule(&trace, ent->client->ps.origin, ent->client->ps.mins,
                        ent->client->ps.maxs, ent->client->ps.origin,
                        ent->client->ps.clientNum, CONTENTS_SOLID);

      if (!trace.allsolid) {
        return true;
      }
    }
  }

  return false;
}

std::string Utilities::timestampToString(int timestamp, const char *format,
                                         const char *start) {
  char buf[1024];
  struct tm *lt = NULL;
  time_t toConvert = timestamp;
  lt = localtime(&toConvert);
  if (timestamp > 0) {
    strftime(buf, sizeof(buf), format, lt);
  } else {
    return start;
  }

  return std::string(buf);
}

std::string Utilities::getPath(const std::string &name) {
  auto osPath = UtilityHelperFunctions::BuildOSPath(name.c_str());
  return osPath ? osPath : std::string();
}

bool Utilities::anyonePlaying() {
  for (auto i = 0; i < level.numConnectedClients; i++) {
    auto clientNum = level.sortedClients[i];
    auto target = g_entities + clientNum;

    if (target->client->sess.sessionTeam != TEAM_SPECTATOR) {
      return true;
    }
  }
  return false;
}

void Utilities::Log(const std::string &message) {
  G_LogPrintf(message.c_str());
}

std::string Utilities::ReadFile(const std::string &filepath) {
  fileHandle_t f;
  auto len = trap_FS_FOpenFile(filepath.c_str(), &f, FS_READ);
  if (len == -1) {
    throw std::runtime_error("Could not open file for reading: " + filepath);
  }

  std::unique_ptr<char[]> buf(new char[len + 1]);
  trap_FS_Read(buf.get(), len, f);
  trap_FS_FCloseFile(f);
  buf[len] = 0;
  return std::string(buf.get());
}

void Utilities::WriteFile(const std::string &filepath,
                          const std::string &content) {
  fileHandle_t f;
  auto len = trap_FS_FOpenFile(filepath.c_str(), &f, FS_WRITE);
  if (len == -1) {
    throw std::runtime_error("Could not open file for writing: " + filepath);
  }

  auto bytesWritten = trap_FS_Write(content.c_str(), content.length(), f);
  if (bytesWritten == 0) {
    trap_FS_FCloseFile(f);
    throw std::runtime_error("Wrote 0 bytes to " + filepath);
  }
  trap_FS_FCloseFile(f);
}

void Utilities::Logln(const std::string &message) {
  G_LogPrintf("%s\n", message.c_str());
}

void Utilities::Console(const std::string &message) {
  G_Printf(message.c_str());
}

void Utilities::Error(const std::string &error) { G_Error(error.c_str()); }

std::vector<std::string> Utilities::getMaps() {
  std::vector<std::string> maps;
  MapStatistics mapStats;

  int i = 0;
  int numDirs = 0;
  int dirLen = 0;
  char dirList[32768];
  char *dirPtr = nullptr;
  numDirs = trap_FS_GetFileList("maps", ".bsp", dirList, sizeof(dirList));
  dirPtr = dirList;

  for (i = 0; i < numDirs; i++, dirPtr += dirLen + 1) {
    dirLen = static_cast<int>(strlen(dirPtr));
    if (strlen(dirPtr) > 4) {
      dirPtr[strlen(dirPtr) - 4] = '\0';
    }

    char buf[MAX_QPATH] = "\0";
    Q_strncpyz(buf, dirPtr, sizeof(buf));
    Q_strlwr(buf);

    if (MapStatistics::isBlockedMap(buf)) {
      continue;
    }

    maps.emplace_back(buf);
  }

  return maps;
}

void Utilities::getOriginOrBmodelCenter(const gentity_t *ent, float dst[3]) {
  // ^ vec3_t is not used to avoid including g_local.h
  if (!VectorCompare(ent->r.currentOrigin, vec3_origin)) {
    VectorCopy(ent->r.currentOrigin, dst);
  } else {
    dst[0] = (ent->r.absmax[0] + ent->r.absmin[0]) / 2;
    dst[1] = (ent->r.absmax[1] + ent->r.absmin[1]) / 2;
    dst[2] = (ent->r.absmax[2] + ent->r.absmin[2]) / 2;
  }
}
