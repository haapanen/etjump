// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// gameinfo.c
//

#include "ui_local.h"

//
// arena and bot info
//

int ui_numBots;
// static char *ui_botInfos[MAX_BOTS];

static int ui_numArenas;

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile(char *filename) {
  int handle;
  pc_token_t token;

  handle = trap_PC_LoadSource(filename);

  if (!handle) {
    trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
    return;
  }

  if (!trap_PC_ReadToken(handle, &token)) {
    trap_PC_FreeSource(handle);
    return;
  }

  if (*token.string != '{') {
    trap_PC_FreeSource(handle);
    return;
  }

  uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
  uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
  uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

  while (trap_PC_ReadToken(handle, &token)) {
    if (*token.string == '}') {

      if (!uiInfo.mapList[uiInfo.mapCount].typeBits) {
        uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
      }

      uiInfo.mapCount++;
      if (uiInfo.mapCount >= MAX_MAPS) {
        break;
      }

      if (!trap_PC_ReadToken(handle, &token)) {
        // eof
        trap_PC_FreeSource(handle);
        return;
      }

      if (*token.string != '{') {
        trap_Print(va(S_COLOR_RED "unexpected token '%s' "
                                  "inside: %s\n",
                      token.string, filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "map")) {
      if (!PC_String_Parse(handle,
                           &uiInfo.mapList[uiInfo.mapCount].mapLoadName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "longname")) {
      if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mapName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "briefing")) {
      if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].briefing)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "lmsbriefing")) {
      if (!PC_String_Parse(handle,
                           &uiInfo.mapList[uiInfo.mapCount].lmsbriefing)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
      /*} else if( !Q_stricmp( token.string,
         "objectives" ) ) { if( !PC_String_Parse(
         handle,
         &uiInfo.mapList[uiInfo.mapCount].objectives ) )
         { trap_Print( va( S_COLOR_RED "unexpected end
         of file inside: %s\n", filename ) );
              trap_PC_FreeSource( handle );
              return;
          }*/
    } else if (!Q_stricmp(token.string, "timelimit")) {
      if (!PC_Int_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].Timelimit)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "axisrespawntime")) {
      if (!PC_Int_Parse(handle,
                        &uiInfo.mapList[uiInfo.mapCount].AxisRespawnTime)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "alliedrespawntime")) {
      if (!PC_Int_Parse(handle,
                        &uiInfo.mapList[uiInfo.mapCount].AlliedRespawnTime)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "type")) {
      if (!trap_PC_ReadToken(handle, &token)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      } else {
        if (strstr(token.string, "wolfsp")) {
          uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_SINGLE_PLAYER);
        }
        if (strstr(token.string, "wolflms")) {
          uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_LMS);
        }
        if (strstr(token.string, "wolfmp")) {
          uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF);
        }
        if (strstr(token.string, "wolfsw")) {
          uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_WOLF_STOPWATCH);
        }
      }
    } else if (!Q_stricmp(token.string, "mapposition_x")) {
      if (!PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[0])) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "mapposition_y")) {
      if (!PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[1])) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    }
  }

  trap_PC_FreeSource(handle);
  return;
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas() {
  int numdirs;
  char filename[128];
  char dirlist[16000];
  char *dirptr;
  int i;
  int dirlen;

  ui_numArenas = 0;
  uiInfo.mapCount = 0;

  // get all arenas from .arena files
  numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 12000);
  dirptr = dirlist;
  for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
    dirlen = static_cast<int>(strlen(dirptr));
    Q_strncpyz(filename, "scripts/", sizeof(filename));
    Q_strcat(filename, sizeof(filename), dirptr);
    UI_LoadArenasFromFile(filename);
  }
}

mapInfo *UI_FindMapInfoByMapname(const char *name) {
  int i;

  if (uiInfo.mapCount == 0) {
    UI_LoadArenas();
  }

  for (i = 0; i < uiInfo.mapCount; i++) {
    if (!Q_stricmp(uiInfo.mapList[i].mapLoadName, name)) {
      return &uiInfo.mapList[i];
    }
  }

  return NULL;
}

// Campaign files
/*
===============
UI_LoadCampaignsFromFile
===============
*/
/*static void UI_LoadCampaignsFromFile( const char *filename, campaignInfo_t
*campaignList, int *campaignCount, int maxCampaigns ) { int handle, i;
    pc_token_t token;

    handle = trap_PC_LoadSource( filename );

    if( !handle ) {
        trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
        return;
    }

    if( !trap_PC_ReadToken( handle, &token ) )
        return;
    if( *token.string != '{' )
        return;

    while ( trap_PC_ReadToken( handle, &token ) ) {
        if( *token.string == '}' ) {
            if( campaignList[*campaignCount].initial ) {
                campaignList[*campaignCount].unlocked = qtrue;
                // Always unlock the initial campaign
            }

            *campaignCount++;

            if( *campaignCount >= maxCampaigns || !trap_PC_ReadToken( handle,
&token ) ) {
                // eof or max campaigns reached
                trap_PC_FreeSource( handle );
                return;
            }

            if( *token.string != '{' ) {
                //campaignList[*campaignCount].order = -1;

                trap_Print( va( S_COLOR_RED "unexpected token '%s' inside:
%s\n", token.string, filename ) ); trap_PC_FreeSource( handle ); return;
            }
        } else if( !Q_stricmp( token.string, "shortname" ) ) {
            if( !PC_String_Parse( handle,
&campaignList[*campaignCount].campaignShortName ) ) { trap_Print( va(
S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                trap_PC_FreeSource( handle );
                return;
            }
        } else if( !Q_stricmp( token.string, "name" ) ) {
            if( !PC_String_Parse( handle,
&campaignList[*campaignCount].campaignName ) ) { trap_Print( va( S_COLOR_RED
"unexpected end of file inside: %s\n", filename ) ); trap_PC_FreeSource( handle
); return;
            }
        } else if( !Q_stricmp( token.string, "description" ) ) {
            if( !PC_String_Parse( handle,
&campaignList[*campaignCount].campaignDescription ) ) { trap_Print( va(
S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                trap_PC_FreeSource( handle );
                return;
            }
        } else if( !Q_stricmp( token.string, "image" ) ) {
            if( !PC_String_Parse( handle,
&campaignList[*campaignCount].campaignShotName ) ) { trap_Print( va( S_COLOR_RED
"unexpected end of file inside: %s\n", filename ) ); trap_PC_FreeSource( handle
); return; } else { campaignList[*campaignCount].campaignShot = -1;
            }
        } else if( !Q_stricmp( token.string, "initial" ) ) {
            campaignList[*campaignCount].initial = qtrue;
        } else if( !Q_stricmp( token.string, "next" ) ) {
            if( !PC_String_Parse( handle,
&campaignList[*campaignCount].nextCampaignShortName ) ) { trap_Print( va(
S_COLOR_RED "unexpected end of file inside: %s\n", filename ) );
                trap_PC_FreeSource( handle );
                return;
            }
        } else if( !Q_stricmp( token.string, "type" ) ) {
            if( !trap_PC_ReadToken( handle, &token ) ) {
                trap_Print( va( S_COLOR_RED "unexpected end of file inside:
%s\n", filename ) ); trap_PC_FreeSource( handle ); return;
            }

            if( strstr( token.string, "wolfsp" ) ) {
                campaignList[*campaignCount].typeBits |= (1 <<
GT_SINGLE_PLAYER);
            }
            if( strstr( token.string, "wolfmp" ) ) {
                campaignList[*campaignCount].typeBits |= (1 << GT_WOLF);
            }
            if( strstr( token.string, "wolfsw" ) ) {
                campaignList[*campaignCount].typeBits |= (1 <<
GT_WOLF_STOPWATCH);
            }
            if( strstr( token.string, "wolflms" ) ) {
                campaignList[*campaignCount].typeBits |= (1 << GT_WOLF_LMS);
            }
        } else if( !Q_stricmp( token.string, "maps" ) ) {
            if( !PC_String_Parse( handle, &campaignList[*campaignCount].maps ) )
{ trap_Print( va( S_COLOR_RED "unexpected end of file inside: %s\n", filename )
); trap_PC_FreeSource( handle ); return;
            }
        }
    }

    trap_PC_FreeSource( handle );
}

static void UI_LinkCampaignsToArenas( void ) {
    int i;

    for( i = 0; i < uiInfo.campaignCount; i++ ) {
        char *ptr, mapname[128], *mapnameptr;

        // find the mapInfo's that match our mapnames
        uiInfo.campaignList[i].mapCount = 0;

        ptr = uiInfo.campaignList[i].maps;
        while( *ptr ) {
            mapnameptr = mapname;
            while( *ptr && *ptr != ';' ) {
                *mapnameptr++ = *ptr++;
            }
            if( *ptr )
                ptr++;
            *mapnameptr = '\0';
            for( i = 0; i < uiInfo.mapCount; i++ ) {
                if( !Q_stricmp( uiInfo.mapList[i].mapLoadName, mapname ) ) {
                    campaignList[*campaignCount].mapInfos[campaignList[*campaignCount].mapCount++]
= &uiInfo.mapList[i]; break;
                }
            }
        }
    }
}*/

static void UI_LoadCampaignsFromFile(const char *filename) {
  int handle, i;
  pc_token_t token;

  handle = trap_PC_LoadSource(filename);

  if (!handle) {
    trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
    return;
  }

  if (!trap_PC_ReadToken(handle, &token)) {
    trap_PC_FreeSource(handle);
    return;
  }
  if (*token.string != '{') {
    trap_PC_FreeSource(handle);
    return;
  }

  while (trap_PC_ReadToken(handle, &token)) {
    if (*token.string == '}') {
      if (uiInfo.campaignList[uiInfo.campaignCount].initial) {
        if (uiInfo.campaignList[uiInfo.campaignCount].typeBits &
            (1 << GT_SINGLE_PLAYER)) {
          uiInfo.campaignList[uiInfo.campaignCount].unlocked = qtrue;
        }
        // Always unlock the initial SP campaign
      }

      uiInfo.campaignList[uiInfo.campaignCount].campaignCinematic = -1;
      uiInfo.campaignList[uiInfo.campaignCount].campaignShot = -1;

      uiInfo.campaignCount++;

      if (!trap_PC_ReadToken(handle, &token)) {
        // eof
        trap_PC_FreeSource(handle);
        return;
      }

      if (*token.string != '{') {
        // uiInfo.campaignList[uiInfo.campaignCount].order
        // = -1;

        trap_Print(va(S_COLOR_RED "unexpected token '%s' "
                                  "inside: %s\n",
                      token.string, filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "shortname")) {
      if (!PC_String_Parse(
              handle,
              &uiInfo.campaignList[uiInfo.campaignCount].campaignShortName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "name")) {
      if (!PC_String_Parse(
              handle,
              &uiInfo.campaignList[uiInfo.campaignCount].campaignName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "description")) {
      if (!PC_String_Parse(
              handle,
              &uiInfo.campaignList[uiInfo.campaignCount].campaignDescription)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "image")) {
      if (!PC_String_Parse(
              handle,
              &uiInfo.campaignList[uiInfo.campaignCount].campaignShotName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      } else {
        uiInfo.campaignList[uiInfo.campaignCount].campaignShot = -1;
      }
    } else if (!Q_stricmp(token.string, "initial")) {
      uiInfo.campaignList[uiInfo.campaignCount].initial = qtrue;
    } else if (!Q_stricmp(token.string, "next")) {
      if (!PC_String_Parse(handle, &uiInfo.campaignList[uiInfo.campaignCount]
                                        .nextCampaignShortName)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }
    } else if (!Q_stricmp(token.string, "type")) {
      if (!trap_PC_ReadToken(handle, &token)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }

      if (strstr(token.string, "wolfsp")) {
        uiInfo.campaignList[uiInfo.campaignCount].typeBits |=
            (1 << GT_SINGLE_PLAYER);
      }
      if (strstr(token.string, "wolfmp")) {
        uiInfo.campaignList[uiInfo.campaignCount].typeBits |= (1 << GT_WOLF);
      }
      if (strstr(token.string, "wolfsw")) {
        uiInfo.campaignList[uiInfo.campaignCount].typeBits |=
            (1 << GT_WOLF_STOPWATCH);
      }
      if (strstr(token.string, "wolflms")) {
        uiInfo.campaignList[uiInfo.campaignCount].typeBits |=
            (1 << GT_WOLF_LMS);
      }
    } else if (!Q_stricmp(token.string, "maps")) {
      char *ptr, mapname[128], *mapnameptr;

      if (!trap_PC_ReadToken(handle, &token)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }

      // find the mapInfo's that match our mapnames
      uiInfo.campaignList[uiInfo.campaignCount].mapCount = 0;

      ptr = token.string;
      while (*ptr) {
        mapnameptr = mapname;
        while (*ptr && *ptr != ';') {
          *mapnameptr++ = *ptr++;
        }
        if (*ptr) {
          ptr++;
        }
        *mapnameptr = '\0';
        for (i = 0; i < uiInfo.mapCount; i++) {
          if (!Q_stricmp(uiInfo.mapList[i].mapLoadName, mapname)) {
            uiInfo.campaignList[uiInfo.campaignCount]
                .mapInfos[uiInfo.campaignList[uiInfo.campaignCount]
                              .mapCount++] = &uiInfo.mapList[i];
            break;
          }
        }
      }
    } else if (!Q_stricmp(token.string, "maptc")) {
      if (!trap_PC_ReadToken(handle, &token)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }

      uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0] = token.floatvalue;

      if (!trap_PC_ReadToken(handle, &token)) {
        trap_Print(va(S_COLOR_RED "unexpected end of file "
                                  "inside: %s\n",
                      filename));
        trap_PC_FreeSource(handle);
        return;
      }

      uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1] = token.floatvalue;

      uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][0] =
          650 + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0];
      uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][1] =
          650 + uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1];
      /*if( !trap_PC_ReadToken( handle, &token ) ) {
          trap_Print( va( S_COLOR_RED "unexpected end of
      file inside: %s\n", filename ) );
      trap_PC_FreeSource( handle ); return;
      }

      uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][0]
      = token.floatvalue +
      uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][0];

      if( !trap_PC_ReadToken( handle, &token ) ) {
          trap_Print( va( S_COLOR_RED "unexpected end of
      file inside: %s\n", filename ) );
      trap_PC_FreeSource( handle ); return;
      }

      uiInfo.campaignList[uiInfo.campaignCount].mapTC[1][1]
      = token.floatvalue +
      uiInfo.campaignList[uiInfo.campaignCount].mapTC[0][1];*/
    }
  }

  trap_PC_FreeSource(handle);
}

const char *UI_DescriptionForCampaign(void) {
  int i = 0, j = 0;
  char *mapname;
  char info[MAX_INFO_STRING];

  trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

  mapname = Info_ValueForKey(info, "mapname");

  for (; i < uiInfo.campaignCount; i++) {
    for (; j < uiInfo.campaignList[i].mapCount; j++) {
      if (!Q_stricmp(mapname, uiInfo.campaignList[i].mapInfos[j]->mapName)) {
        return uiInfo.campaignList[i].campaignDescription;
      }
    }
  }

  return NULL;
}

const char *UI_NameForCampaign(void) {
  int i = 0, j = 0;
  char *mapname;
  char info[MAX_INFO_STRING];

  trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

  mapname = Info_ValueForKey(info, "mapname");

  for (; i < uiInfo.campaignCount; i++) {
    for (; j < uiInfo.campaignList[i].mapCount; j++) {
      if (!Q_stricmp(mapname, uiInfo.campaignList[i].mapInfos[j]->mapName)) {
        return uiInfo.campaignList[i].campaignName;
      }
    }
  }

  return NULL;
}

/*
===============
UI_FindCampaignInCampaignList
===============
*/
int UI_FindCampaignInCampaignList(const char *shortName) {
  int i;

  if (!shortName) {
    return (-1);
  }

  for (i = 0; i < uiInfo.campaignCount; i++) {
    if (!Q_stricmp(uiInfo.campaignList[i].campaignShortName, shortName)) {
      return (i);
    }
  }

  return (-1);
}

/*
===============
UI_LoadCampaigns
===============
*/
void UI_LoadCampaigns() {
  int numdirs;
  char filename[128];
  char dirlist[1024];
  char *dirptr;
  int i, j;
  int dirlen;
  long hash;
  const char *ch;

  uiInfo.campaignCount = 0;
  memset(&uiInfo.campaignList, 0, sizeof(uiInfo.campaignList));

  // get all campaigns from .campaign files
  numdirs = trap_FS_GetFileList("scripts", ".campaign", dirlist, 1024);
  dirptr = dirlist;
  for (i = 0; i < numdirs && uiInfo.campaignCount < MAX_CAMPAIGNS;
       i++, dirptr += dirlen + 1) {
    dirlen = static_cast<int>(strlen(dirptr));
    Q_strncpyz(filename, "scripts/", sizeof(filename));
    Q_strcat(filename, sizeof(filename), dirptr);
    UI_LoadCampaignsFromFile(filename);
  }
  //// JPW
  // NERVE pulled per atvi req
  if (UI_OutOfMemory()) {
    trap_Print(S_COLOR_YELLOW
               "WARNING: not enough memory in pool to load all campaigns\n");
  }

  // Sort the campaigns for single player

  // first, find the initial campaign
  for (i = 0; i < uiInfo.campaignCount; i++) {
    if (!(uiInfo.campaignList[i].typeBits & (1 << GT_SINGLE_PLAYER))) {
      continue;
    }

    if (uiInfo.campaignList[i].initial) {
      uiInfo.campaignList[i].order = 0;
      break;
    }
  }

  // now use the initial nextCampaignShortName to find the next one,
  // etc. for single player campaigns
  // rain - don't let it go above the maximum number of campaigns
  while (i < MAX_CAMPAIGNS) {
    j = UI_FindCampaignInCampaignList(
        uiInfo.campaignList[i].nextCampaignShortName);

    if (j == -1) {
      break;
    }

    uiInfo.campaignList[j].order = uiInfo.campaignList[i].order + 1;
    i = j;
  }

  // Load the campaign save
  BG_LoadCampaignSave(va("profiles/%s/campaign.dat", cl_profile.string),
                      &uiInfo.campaignStatus, cl_profile.string);

  for (i = 0; i < uiInfo.campaignCount; i++) {
    // generate hash for campaign shortname
    for (hash = 0, ch = uiInfo.campaignList[i].campaignShortName; *ch != '\0';
         ch++) {
      hash += static_cast<long>(
          (tolower(*ch)) *
          ((ch - uiInfo.campaignList[i].campaignShortName) + 119));
    }

    // find the entry in the campaignsave
    for (j = 0; j < uiInfo.campaignStatus.header.numCampaigns; j++) {
      if (hash == uiInfo.campaignStatus.campaigns[j].shortnameHash) {
        uiInfo.campaignList[i].unlocked = qtrue;
        uiInfo.campaignList[i].progress =
            uiInfo.campaignStatus.campaigns[j].progress;
        uiInfo.campaignList[i].cpsCampaign =
            &uiInfo.campaignStatus.campaigns[j];
      }
    }
  }
}
