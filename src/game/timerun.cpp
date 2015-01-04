#include "timerun.hpp"
#include "g_local.hpp"
#include "g_utilities.hpp"
#include <sqlite3.h>
#include "admin/session.hpp"
#include "g_save.hpp"

Timerun::Timerun(Session* session, SaveSystem *saveSystem) : session_(session), saveSystem_(saveSystem)
{
}

Timerun::~Timerun()
{
}

void Timerun::Initialize()
{
    if (strlen(g_timerunsDatabase.string) == 0)
    {
        return;
    }

    std::string dbPath = GetPath(g_timerunsDatabase.string);
    sqlite3 *db = NULL;
    sqlite3_open(dbPath.c_str(), &db);

    char *err;
    int rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS record (id INTEGER PRIMARY KEY AUTOINCREMENT, time INT NOT NULL, map VARCHAR(256), run VARCHAR(256), player INT NOT NULL, player_name VARCHAR(256));", 0, 0, &err);
    if (rc != SQLITE_OK)
    {
        G_LogPrintf("ERROR: couldn't create timerun database table. (%d): %s\n", rc, err);
        return;
    }

    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(db, "SELECT id, time, map, run, player, player_name FROM record WHERE map=?;",-1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        G_LogPrintf("ERROR: couldn't prepare timerun query. (%d): %s\n", rc, sqlite3_errmsg(db));
        return;
    }

    rc = sqlite3_bind_text(stmt, 1, level.rawmapname, strlen(level.rawmapname), SQLITE_STATIC);
    if (rc != SQLITE_OK)
    {
        G_LogPrintf("ERROR: couldn't bind map name to records query. (%d): %s\n",
            rc, sqlite3_errmsg(db));
        return;
    }

    rc = sqlite3_step(stmt);
    while (rc != SQLITE_DONE)
    {
        boost::shared_ptr<Record> record(new Record);
        RunIterator it;
        const char *val = NULL;
        switch (rc)
        {
        case SQLITE_ROW:
            record->time = sqlite3_column_int(stmt, 1);
            val = (const char*)sqlite3_column_text(stmt, 2);
            record->map = val ? val : "";
            val = (const char *)sqlite3_column_text(stmt, 3);
            record->run = val ? val : "";
            record->player = sqlite3_column_int(stmt, 4);
            val = (const char*)sqlite3_column_text(stmt, 5);
            record->playerName = val ? val : "";
            it = records_.find(record->run);
            if (it != records_.end())
            {
                it->second.push_back(record);
            }
            else
            {
                records_[record->run] = std::vector<boost::shared_ptr<Record> >();
                records_[record->run].push_back(record);
            }
            G_LogPrintf((boost::format("%s\n") % *(record.get())).str().c_str());
            break;
        default:
            G_LogPrintf("ERROR: couldn't fetch a row from database. (%d): %s\n", rc, sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    G_LogPrintf("Successfully initialized timerun records database.\n");
}

void Timerun::Shutdown()
{
}

void Timerun::StartTimer(const char* runName, gentity_t* ent)
{
    Player *player = &players_[ClientNum(ent)];

    if (!player->racing)
    {
        player->racing = true;
        player->userDatabaseId = game.session->GetId(ent);
        player->currentName = ent->client->pers.netname;
        player->raceStartTime = ent->client->ps.commandTime;
        player->time = 0;
        player->runName = runName ? runName : "";

        saveSystem_->ResetSavedPositions(ent);
    }
}

void Timerun::StopTimer(const char* runName, gentity_t* ent)
{
    Player *player = &players_[ClientNum(ent)];

    if (player->racing)
    {
        int millis = ent->client->ps.commandTime - player->raceStartTime;
        int seconds = millis / 1000;
        millis = millis - seconds * 1000;
        int minutes = seconds / 60;
        seconds = seconds - minutes * 60;

        player->time = ent->client->ps.commandTime - player->raceStartTime;
        CPMAll((boost::format("Player %s ^7Finished %s ^7in %02d:%02d:%03d") % player->currentName % player->runName % minutes % seconds % millis).str());
        InsertRecord(level.rawmapname, player);

        player->racing = false;
        player->runName = "";
    }
}

void Timerun::PrintRecords(gentity_t* ent)
{
    RunIterator it = records_.begin();
    RunIterator end = records_.end();

    boost::format fmt("%s %s %s %d\n");
    BufferPrinter printer(ent);
    printer.Begin();
    for (; it != end; it++)
    {
        RecordIterator rit = it->second.begin();
        RecordIterator ritEnd = it->second.end();

        for (; rit != ritEnd; rit++)
        {
            printer.Print((fmt
                % rit->get()->map
                % rit->get()->run
                % rit->get()->playerName
                % rit->get()->time).str());
        }
    }
    printer.Finish(false);
}


// Inserts a record to database. Checks if user already has a record
// and if it does, just updates current record
void Timerun::InsertRecord(std::string mapName, Player* player)
{
    std::string runName = player->runName;
    RunIterator rit = records_.find(runName);
    RunIterator ritEnd = records_.end();
    bool update = false;
    // Check if the run has any records
    if (rit != ritEnd)
    {
        RecordIterator it = rit->second.begin();
        RecordIterator end = rit->second.end();

        // Let's try to find players record
        for (; it != end; it++)
        {
            if ((*it)->player == player->userDatabaseId)
            {
                // Ok we found the record.
                // check that it's faster than current record in db
                if (player->time < (*it)->time)
                {
                    // New record is faster
                    (*it)->time = player->time;
                    (*it)->playerName = player->currentName;
                    update = true;
                    break;
                }
                // Old record was faster, do nothing
                return;
            }
        }
    }
    else
    {
        // Couldn't find a record with that run name so let's add it 
        records_[runName] = std::vector<boost::shared_ptr<Record> >();

        boost::shared_ptr<Record> newRecord(new Record);
        newRecord->map = mapName;
        newRecord->player = player->userDatabaseId;
        newRecord->playerName = player->currentName;
        newRecord->run = player->runName;
        newRecord->time = player->time;

        records_[runName].push_back(newRecord);
    }

   
    InsertRecordOperation *op = new InsertRecordOperation(mapName, *player, update);
    op->RunAndDeleteObject();
    return;
}

Timerun::InsertRecordOperation::InsertRecordOperation(std::string mapName, Player player, bool update) : mapName_(mapName), player_(player), update_(update)
{
}

Timerun::InsertRecordOperation::~InsertRecordOperation()
{
}

void Timerun::InsertRecordOperation::Execute()
{
    const std::string op = "Insert Record Operation";
    if (!OpenDatabase(g_timerunsDatabase.string))
    {
        PrintOpenError(op);
        return;
    }

    if (update_)
    {
        if (!PrepareStatement("UPDATE record SET time=?, player_name=? WHERE map=? AND run=? AND player=?;"))
        {
            PrintPrepareError(op);
            return;
        }

        if (!BindInt(1, player_.time) ||
            !BindString(2, player_.currentName) ||
            !BindString(3, mapName_) ||
            !BindString(4, player_.runName) ||
            !BindInt(5, player_.userDatabaseId)) {
            PrintBindError(op);
            return;
        }

        if (!ExecuteStatement())
        {
            PrintExecuteError(op);
            return;
        }
    }
    else
    {
        if (!PrepareStatement("INSERT INTO record (time, map, run, player, player_name) VALUES (?, ?, ?, ?, ?);"))
        {
            PrintPrepareError(op);
            return;
        }

        if (!BindInt(1, player_.time) ||
            !BindString(2, mapName_) ||
            !BindString(3, player_.runName) ||
            !BindInt(4, player_.userDatabaseId) ||
            !BindString(5, player_.currentName)) {
            PrintBindError(op);
            return;
        }

        if (!ExecuteStatement())
        {
            PrintExecuteError(op);
            return;
        }
    }

    if (update_)
    {
        G_LogPrintf("Updated %s's record time to %d\n", player_.currentName.c_str(), player_.time);
    }
    else
    {
        G_LogPrintf("Inserted %s's record time %d\n", player_.currentName.c_str(), player_.time);
    }
}