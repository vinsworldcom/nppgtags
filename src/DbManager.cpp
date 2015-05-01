/**
 *  \file
 *  \brief  GTags database manager
 *
 *  \author  Pavel Nedev <pg.nedev@gmail.com>
 *
 *  \section COPYRIGHT
 *  Copyright(C) 2014-2015 Pavel Nedev
 *
 *  \section LICENSE
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define WIN32_LEAN_AND_MEAN
#include "DbManager.h"
#include <windows.h>


namespace GTags
{

DbManager DbManager::Instance;


/**
 *  \brief
 */
DbHandle DbManager::RegisterDb(const CPath& dbPath, bool writeEn)
{
    AUTOLOCK(_lock);

    for (std::list<GTagsDb>::iterator dbi = _dbList.begin();
        dbi != _dbList.end(); dbi++)
    {
        if (dbi->_path == dbPath)
        {
            if (dbi->IsLocked())
            {
                return NULL;
            }
            else
            {
                dbi->Lock(writeEn);
                return &(dbi->_path);
            }
        }
    }

    return addDb(dbPath, writeEn);
}


/**
 *  \brief
 */
bool DbManager::UnregisterDb(DbHandle db)
{
    if (!db)
        return false;

    bool ret = false;

    AUTOLOCK(_lock);

    for (std::list<GTagsDb>::iterator dbi = _dbList.begin();
        dbi != _dbList.end(); dbi++)
    {
        if (db == &(dbi->_path))
        {
            dbi->Unlock();
            if (!dbi->IsLocked())
            {
                ret = deleteDb(dbi->_path);
                _dbList.erase(dbi);
            }
            break;
        }
    }

    return ret;
}


/**
 *  \brief
 */
DbHandle DbManager::GetDb(const CPath& filePath, bool writeEn, bool* success)
{
    if (!success)
        return NULL;

    AUTOLOCK(_lock);

    *success = false;

    DbHandle db = lockDb(filePath, writeEn, success);
    if (db)
        return db;

    CPath dbPath(filePath);

    int len;
    while ((len = dbPath.Up()))
        if (DbExistsInFolder(dbPath))
            break;

    if (len == 0)
        return NULL;

    *success = true;

    return addDb(dbPath, writeEn);
}


/**
 *  \brief
 */
bool DbManager::PutDb(DbHandle db)
{
    if (!db)
        return false;

    AUTOLOCK(_lock);

    for (std::list<GTagsDb>::iterator dbi = _dbList.begin();
        dbi != _dbList.end(); dbi++)
    {
        if (db == &(dbi->_path))
        {
            dbi->Unlock();
            return dbi->IsLocked();
        }
    }

    return false;
}


/**
 *  \brief
 */
bool DbManager::DbExistsInFolder(const CPath& folder)
{
    CPath db(folder);
    db += _T("GTAGS");
    return db.FileExists();
}


/**
 *  \brief
 */
bool DbManager::deleteDb(CPath& dbPath)
{
    dbPath += _T("GTAGS");
    BOOL ret = DeleteFile(dbPath.C_str());
    if (ret)
    {
        dbPath.StripFilename();
        dbPath += _T("GPATH");
        ret = DeleteFile(dbPath.C_str());
    }
    if (ret)
    {
        dbPath.StripFilename();
        dbPath += _T("GRTAGS");
        ret = DeleteFile(dbPath.C_str());
    }

    return ret ? true : false;
}


/**
 *  \brief
 */
DbHandle DbManager::addDb(const CPath& dbPath, bool writeEn)
{
    GTagsDb newDb(dbPath, writeEn);
    _dbList.push_back(newDb);

    return &(_dbList.rbegin()->_path);
}


/**
 *  \brief
 */
DbHandle DbManager::lockDb(const CPath& filePath, bool writeEn, bool* success)
{
    for (std::list<GTagsDb>::iterator dbi = _dbList.begin();
        dbi != _dbList.end(); dbi++)
    {
        if (dbi->_path.Contains(filePath))
        {
            if (!DbExistsInFolder(dbi->_path))
            {
                _dbList.erase(dbi);
                return NULL;
            }
            *success = dbi->Lock(writeEn);
            return &(dbi->_path);
        }
    }

    return NULL;
}

} // namespace GTags
