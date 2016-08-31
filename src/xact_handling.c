/* ------------------------------------------------------------------------
 *
 * xact_handling.c
 *		Transaction-specific locks and other functions
 *
 * Copyright (c) 2016, Postgres Professional
 *
 * ------------------------------------------------------------------------
 */

#include "xact_handling.h"

#include "postgres.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "miscadmin.h"
#include "storage/lmgr.h"


static inline void SetLocktagRelationOid(LOCKTAG *tag, Oid relid);
static inline bool do_we_hold_the_lock(Oid relid, LOCKMODE lockmode);


/*
 * Lock certain partitioned relation to disable concurrent access.
 */
void
xact_lock_partitioned_rel(Oid relid)
{
	/* Share exclusive lock conflicts with itself */
	LockRelationOid(relid, ShareUpdateExclusiveLock);
}

/*
 * Unlock partitioned relation.
 */
void
xact_unlock_partitioned_rel(Oid relid)
{
	UnlockRelationOid(relid, ShareUpdateExclusiveLock);
}

/*
 * Lock certain relation's data (INSERT | UPDATE | DELETE).
 */
void
xact_lock_rel_data(Oid relid)
{
	LockRelationOid(relid, ShareLock);
}

/*
 * Unlock relation's data.
 */
void
xact_unlock_rel_data(Oid relid)
{
	UnlockRelationOid(relid, ShareLock);
}

/*
 * Check whether we already hold a lock that
 * might conflict with partition spawning BGW.
 */
bool
xact_bgw_conflicting_lock_exists(Oid relid)
{
	LOCKMODE	lockmode;

	/* Try each lock >= ShareUpdateExclusiveLock */
	for (lockmode = ShareUpdateExclusiveLock;
		 lockmode <= AccessExclusiveLock;
		 lockmode++)
	{
		if (do_we_hold_the_lock(relid, lockmode))
			return true;
	}

	return false;
}

/*
 * Check if table is being modified
 * concurrently in a separate transaction.
 */
bool
xact_is_table_being_modified(Oid relid)
{
	/*
	 * Check if someone has already started a
	 * transaction and modified table's contents.
	 */
	if (ConditionalLockRelationOid(relid, ExclusiveLock))
	{
		UnlockRelationOid(relid, ExclusiveLock);
		return false;
	}

	return true;
}

/*
 * Check if current transaction's level is READ COMMITTED.
 */
bool
xact_is_level_read_committed(void)
{
	if (XactIsoLevel <= XACT_READ_COMMITTED)
		return true;

	return false;
}

/*
 * Do we hold the specified lock?
 */
static inline bool
do_we_hold_the_lock(Oid relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	/* Create a tag for lock */
	SetLocktagRelationOid(&tag, relid);

	/* If lock is alredy held, release it one time (decrement) */
	switch (LockAcquire(&tag, lockmode, false, true))
	{
		case LOCKACQUIRE_ALREADY_HELD:
			LockRelease(&tag, lockmode, false);
			return true;

		case LOCKACQUIRE_OK:
			LockRelease(&tag, lockmode, false);
			return false;

		default:
			return false; /* should not happen */
	}
}

/*
 * SetLocktagRelationOid
 *		Set up a locktag for a relation, given only relation OID
 */
static inline void
SetLocktagRelationOid(LOCKTAG *tag, Oid relid)
{
	Oid			dbid;

	if (IsSharedRelation(relid))
		dbid = InvalidOid;
	else
		dbid = MyDatabaseId;

	SET_LOCKTAG_RELATION(*tag, dbid, relid);
}