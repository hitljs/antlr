/** \file
 * Provides a number of useful functions that are roughly equivalent
 * to java HashTable and List for the purposes of Antlr 3 C runtime.
 * Also useable by the C programmer for things like symbol tables pointers
 * and so on.
 *
 */
#include    <antlr3.h>

/* Interface functions for hash table
 */
static void		    antlr3HashDelete    (pANTLR3_HASH_TABLE table, void * key);
static void *		    antlr3HashGet	(pANTLR3_HASH_TABLE table, void * key);
static pANTLR3_HASH_ENTRY   antlr3HashRemove    (pANTLR3_HASH_TABLE table, void * key);
static ANTLR3_INT32	    antlr3HashPut	(pANTLR3_HASH_TABLE table, void * key, void * element, void (*freeptr)(void *));
static void		    antlr3HashFree	(pANTLR3_HASH_TABLE table);
static ANTLR3_UINT64	    antlr3HashSize	(pANTLR3_HASH_TABLE table);

/* Interface functions for enumeration
 */
static int	    antlr3EnumNext	    (pANTLR3_HASH_ENUM en, void ** key, void ** data);
static void	    antlr3EnumFree	    (pANTLR3_HASH_ENUM en);

/* Interface functions for List
 */
static void		antlr3ListFree	(pANTLR3_LIST list);
static void		antlr3ListDelete(pANTLR3_LIST list, ANTLR3_UINT64 key);
static void *		antlr3ListGet	(pANTLR3_LIST list, ANTLR3_UINT64 key);
static ANTLR3_INT32	antlr3ListPut	(pANTLR3_LIST list, ANTLR3_UINT64 key, void * element, void (*freeptr)(void *));
static ANTLR3_INT32	antlr3ListAdd   (pANTLR3_LIST list, void * element, void (*freeptr)(void *));
static void *		antlr3ListRemove(pANTLR3_LIST list, ANTLR3_UINT64 key);
static ANTLR3_UINT64	antlr3ListSize	(pANTLR3_LIST list);

/* Interface functions for Stack
 */
static void		antlr3StackFree	(pANTLR3_STACK  stack);
static void		antlr3StackPop	(pANTLR3_STACK	stack);
static void *		antlr3StackGet	(pANTLR3_STACK	stack, ANTLR3_UINT64 key);
static ANTLR3_BOOLEAN	antlr3StackPush	(pANTLR3_STACK	stack, void * element, void (*freeptr)(void *));
static ANTLR3_UINT64	antlr3StackSize	(pANTLR3_STACK	stack);
static void *		antlr3StackPeek	(pANTLR3_STACK	stack);

/* Interface functions for vectors
 */
static	void		antlr3VectorFree    (pANTLR3_VECTOR vector);
static	void		antlr3VectorDel	    (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry);
static	void *		antlr3VectorGet     (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry);
static	void *		antrl3VectorRemove  (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry);
static	ANTLR3_INT32    antlr3VectorAdd	    (pANTLR3_VECTOR vector, void * element, void (*freeptr)(void *));
static	ANTLR3_INT32    antlr3VectorPut	    (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry, void * element, void (*freeptr)(void *));
static	ANTLR3_UINT64   antlr3VectorSize    (pANTLR3_VECTOR vector);

static  void		closeVectorFactory  (pANTLR3_VECTOR_FACTORY factory);
static	pANTLR3_VECTOR  newVector	    (pANTLR3_VECTOR_FACTORY factory);

/* Local function to advance enumeration structure pointers
 */
static void antlr3EnumNextEntry(pANTLR3_HASH_ENUM en);

pANTLR3_HASH_TABLE
antlr3HashTableNew(ANTLR3_UINT32 sizeHint)
{
    /* All we have to do is create the hashtable tracking structure
     * and allocate memory for the requested number of buckets.
     */
    pANTLR3_HASH_TABLE	table;
    
    ANTLR3_UINT32	bucket;	/* Used to traverse the buckets	*/

    table   = ANTLR3_MALLOC(sizeof(ANTLR3_HASH_TABLE));

    /* Error out if no memory left */
    if	(table	== NULL)
    {
	return	(pANTLR3_HASH_TABLE) ANTLR3_ERR_NOMEM;
    }

    /* Allocate memory for the buckets
     */
    table->buckets = (pANTLR3_HASH_BUCKET) ANTLR3_MALLOC((size_t) (sizeof(ANTLR3_HASH_BUCKET) * sizeHint)); 

    if	(table->buckets == NULL)
    {
	ANTLR3_FREE((void *)table);
	return	(pANTLR3_HASH_TABLE) ANTLR3_ERR_NOMEM;
    }

    /* Modulo of the table, (bucket count).
     */
    table->modulo   = sizeHint;

    table->count    = 0;	    /* Nothing in there yet ( I hope)	*/

    /* Initialize the buckets to empty
     */
    for	(bucket = 0; bucket < sizeHint; bucket++)
    {
	table->buckets[bucket].entries = NULL;
    }

    /* Exclude duplicate entries by default
     */
    table->allowDups	= ANTLR3_FALSE;

    /* Install the interface
     */
    table->free		=  antlr3HashFree;
    table->get		=  antlr3HashGet;
    table->put		=  antlr3HashPut;
    table->del		=  antlr3HashDelete;
    table->size		=  antlr3HashSize;
    table->remove	=  antlr3HashRemove;

    return  table;
}

static void
antlr3HashFree(pANTLR3_HASH_TABLE table)
{
    ANTLR3_UINT32	bucket;	/* Used to traverse the buckets	*/

    pANTLR3_HASH_BUCKET	thisBucket;
    pANTLR3_HASH_ENTRY	entry;
    pANTLR3_HASH_ENTRY	nextEntry;

    /* Free the table, all buckets and all entries, and all the
     * keys and data (if the table exists)
     */
    if	(table	!= NULL)
    {
	for	(bucket = 0; bucket < table->modulo; bucket++)
	{
	    thisBucket	= &(table->buckets[bucket]);

	    /* Allow sparse tables, though we don't create them as such at present
	     */
	    if	( thisBucket != NULL)
	    {
		entry	= thisBucket->entries;

		/* Search all entries in the bucket and free them up
		 */
		while	(entry != NULL)
		{
		    /* Save next entry - we do not want to access memory in entry after we
		     * have freed it.
		     */
		    nextEntry	= entry->nextEntry;

		    /* Free any data pointer, this only happens if the user supplied
		     * a pointer to a routine that knwos how to free the structure they
		     * added to the table.
		     */
		    if	(entry->free != NULL)
		    {
			entry->free(entry->data);
		    }

		    /* Free the key memory - we know that we allocated this
		     */
		    if	(entry->key != NULL)
		    {
			ANTLR3_FREE(entry->key);
		    }

		    /* Free this entry
		     */
		    ANTLR3_FREE(entry);
		    entry   = nextEntry;    /* Load next pointer to see if we shoud free it */
		}
		/* Invalidate the current pointer
		 */
		thisBucket->entries = NULL;
	    }
	}

	/* Now we can free the bucket memory
	 */
	ANTLR3_FREE(table->buckets);
    }

    /* Now we free teh memory for the table itself
     */
    ANTLR3_FREE(table);
}

/** return the current size of the hash table
 */
static ANTLR3_UINT64	antlr3HashSize	    (pANTLR3_HASH_TABLE table)
{
    return  table->count;
}

/** Remove the element in the hash table for a particular
 *  key value, if it exists - no error if it does not.
 */
static pANTLR3_HASH_ENTRY
antlr3HashRemove(pANTLR3_HASH_TABLE table, void * key)
{
    ANTLR3_UINT32	    hash;
    pANTLR3_HASH_BUCKET	    bucket;
    pANTLR3_HASH_ENTRY	    entry;
    pANTLR3_HASH_ENTRY	    * nextPointer;

    /* First we need to know the hash of the provided key
     */
    hash    = antlr3Hash(key, (ANTLR3_UINT32)strlen((const char *)key));

    /* Knowing the hash, we can find the bucket
     */
    bucket  = table->buckets + (hash % table->modulo);

    /* Now, we traverse the entries in the bucket until
     * we find the key or the end of the entires in the bucket. 
     * We track the element prior to the one we are exmaining
     * as we need to set its next pointer to the next pointer
     * of the entry we are deleting (if we find it).
     */
    entry	    =   bucket->entries;    /* Entry to examine					    */
    nextPointer	    = & bucket->entries;    /* Where to put the next pointer of the deleted entry   */

    while   (entry != NULL)
    {
	/* See if this is the entry we wish to delete
	 */
	if  (strcmp((const char *)key, (const char *)entry->key) == 0)
	{
	    /* It was the correct entry, so we set the next pointer
	     * of the previous entry to the next pointer of this
	     * located one, which takes it out of the chain.
	     */
	    (*nextPointer)		= entry->nextEntry;

	    /* Release the key - we allocated that
	     */
	    ANTLR3_FREE(entry->key);
	    entry->key	= NULL;

	    table->count--;

	    return entry;
	}
	else
	{
	    /* We found an entry but it wasn't the one that was wanted, so
	     * move to the next one, if any.
	     */
	    nextPointer	= & (entry->nextEntry);	    /* Address of the next pointer in the current entry	    */
	    entry	= entry->nextEntry;	    /* Address of the next element in the bucket (if any)   */
	}
    }

    return NULL;  /* Not found */
}

/** Takes the element with the supplied key out of the list, and deletes the data
 *  calling the supplied free() routine if any. 
 */
static void
antlr3HashDelete    (pANTLR3_HASH_TABLE table, void * key)
{
    pANTLR3_HASH_ENTRY	entry;

    entry = antlr3HashRemove(table, key);
	
    /* Now we can free the elements and the entry in order
     */
    if	(entry != NULL && entry->free != NULL)
    {
	/* Call programmer supplied function to release this entry data
	 */
	entry->free(entry->data);
	entry->data = NULL;
    }
    /* Finally release the space for this entry block.
     */
    ANTLR3_FREE(entry);
}

/** Return the element pointer in the hash table for a particular
 *  key value, or NULL if it don't exist (or was itself NULL).
 */
static void *
antlr3HashGet(pANTLR3_HASH_TABLE table, void * key)
{
    ANTLR3_UINT32	    hash;
    pANTLR3_HASH_BUCKET	    bucket;
    pANTLR3_HASH_ENTRY	    entry;


    /* First we need to know the hash of the provided key
     */
    hash    = antlr3Hash(key, (ANTLR3_UINT32)strlen((const char *)key));

    /* Knowing the hash, we can find the bucket
     */
    bucket  = table->buckets + (hash % table->modulo);

    /* Now we can inspect the key at each entry in the bucket
     * and see if we have a match.
     */
    entry   = bucket->entries;

    while   (entry != NULL)
    {
	if  (strcmp((const char *)key, (const char *)entry->key) == 0)
	{
	    /* Match was found, return the data pointer for this entry
	     */
	    return  entry->data;
	}
	entry = entry->nextEntry;
    }

    /* If we got here, then we did not find the key
     */
    return  NULL;
}



/** Add the element pointer in to the table, based upon the 
 *  hash of the provided key.
 */
static	ANTLR3_INT32
antlr3HashPut(pANTLR3_HASH_TABLE table, void * key, void * element, void (*freeptr)(void *))
{
    ANTLR3_UINT32	    hash;
    pANTLR3_HASH_BUCKET	    bucket;
    pANTLR3_HASH_ENTRY	    entry;
    pANTLR3_HASH_ENTRY	    * newPointer;

    /* First we need to know the hash of the provided key
     */
    hash    = antlr3Hash(key, (ANTLR3_UINT32)strlen((const char *)key));

    /* Knowing the hash, we can find the bucket
     */
    bucket  = table->buckets + (hash % table->modulo);

    /* Knowign the bucket, we can traverse the entries until we
     * we find a NULL pointer ofr we find that this is already 
     * in the table and duplicates were not allowed.
     */
    newPointer	= &bucket->entries;

    while   (*newPointer !=  NULL)
    {
	/* The value at new pointer is pointing to an existing entry.
	 * If duplicates are allowed then we don't care what it is, but
	 * must reject this add if the key is the same as the one we are
	 * supplied with.
	 */
	if  (table->allowDups == ANTLR3_FALSE)
	{
	    if	(strcmp((const char*) key, (const char *)(*newPointer)->key) == 0)
	    {
		return	ANTLR3_ERR_HASHDUP;
	    }
	}

	/* Point to the next entry pointer of the current entry we
	 * are traversing, if it is NULL we will create our new
	 * structure and point this to it.
	 */
	newPointer = &((*newPointer)->nextEntry);
    }

    /* newPointer is now poiting at the pointer where we need to
     * add our new entry, so let's crate the entry and add it in.
     */
    entry   = (pANTLR3_HASH_ENTRY)ANTLR3_MALLOC((size_t)sizeof(ANTLR3_HASH_ENTRY));

    if	(entry == NULL)
    {
	return	ANTLR3_ERR_NOMEM;
    }
	
    entry->data		= element;		/* Install the data element supplied		    */
    entry->free		= freeptr;		/* Function that knows how to release the entry	    */
    entry->key		= ANTLR3_STRDUP(key);	/* Record the key value				    */
    entry->nextEntry	= NULL;			/* Ensure that the forward pointer ends the chain   */

    *newPointer	= entry;    /* Install the next entry in this bucket	*/

    table->count++;

    return  ANTLR3_SUCCESS;
}

/** \brief Creates an enumeration structure to traverse the hash table.
 *
 * \param table Table to enumerate
 * \return Pointer to enumeration structure.
 */
pANTLR3_HASH_ENUM
antlr3EnumNew	(pANTLR3_HASH_TABLE table)
{
    pANTLR3_HASH_ENUM	en;

    /* Allocate structure memory
     */
    en    = (pANTLR3_HASH_ENUM) ANTLR3_MALLOC((size_t)sizeof(ANTLR3_HASH_ENUM));

    /* Check that the allocation was good 
     */
    if	(en == NULL)
    {
	return	(pANTLR3_HASH_ENUM) ANTLR3_ERR_NOMEM;
    }
    
    /* Initialize the start pointers
    */
    en->table	= table;
    en->bucket	= 0;				/* First bucket		    */
    en->entry	= en->table->buckets->entries;	/* First entry to return    */

    /* Special case in that the first bucket may not have anything in it
     * but the antlr3EnumNext() function expects that the en->entry is
     * set to the next valid pointer. Hence if it is not a valid element
     * pointer, attempt to find the next one that is, (table may be empty
     * of course.
     */
    if	(en->entry == NULL)
    {
	antlr3EnumNextEntry(en);
    }

    /* Install the interface
     */
    en->free	=  antlr3EnumFree;
    en->next	=  antlr3EnumNext;

    /* All is good
     */
    return  en;
}

/** \brief Return the next entry in the hashtable ebign traversed by the supplied
 *         enumeration.
 *
 * \param[in] en Pointer to the enumeration tracking structure
 * \param key	 Pointer to void pointer, where the key pointer is returned.
 * \param data	 Pointer to void pointer where the data poitner is returned.
 * \return 
 *	- ANTLR3_SUCCESS if there was a next key
 *	- ANTLR3_FAIL	 if there were no more keys
 *
 * \remark
 *  No checking of input structure is performed!
 */
static int
antlr3EnumNext	(pANTLR3_HASH_ENUM en, void ** key, void ** data)
{
    /* If the current entry is valid, then use it
     */
    if  (en->bucket >= en->table->modulo)
    {
        /* Already exhausted the table
         */
        return	ANTLR3_FAIL;
    }

    /* Pointers are already set to the current entry to return, or
     * we would not be at this point in the logic flow.
     */
    *key	= en->entry->key;
    *data	= en->entry->data;

    /* Return pointers are set up, so now we move the element
     * pointer to the next in the table (if any).
     */
    antlr3EnumNextEntry(en);

    return	ANTLR3_SUCCESS;
}

/** \brief Local function to avance the entry pointer of an enumeration 
 * structure to the next vlaid entry (if there is one).
 *
 * \param[in] enum Pointer to ANTLR3 enumeratio structure returned by antlr3EnumNew()
 *
 * \remark
 *   - The function always leaves the pointers pointing at a valid enrty if there
 *     is one, so if the entry pointer is NULL when this function exits, there were
 *     no more entries in the table.
 */
static void
antlr3EnumNextEntry(pANTLR3_HASH_ENUM en)
{
    pANTLR3_HASH_BUCKET	bucket;

    /* See if the current entry pointer is valid fisrt of all
     */
    if	(en->entry != NULL)
    {
	/* Current entry was a vlaid point, see if ther eis another
	 * one in the chain.
	 */
	if  (en->entry->nextEntry != NULL)
	{
	    /* Next entry in the enumeration is just the next entry
	     * in the chain.
	     */
	    en->entry = en->entry->nextEntry;
	    return;
	}
    }

    /* There were no more entries in the current bucket, if there are
     * more buckets then chase them until we find an entry.
     */
    en->bucket++;

    while   (en->bucket < en->table->modulo)
    {
	/* There was one more bucket, see if it has any elements in it
	 */
	bucket	= en->table->buckets + en->bucket;

	if  (bucket->entries != NULL)
	{
	    /* There was an entry in this bucket, so we can use it
	     * for the next entry in the enumeration.
	     */
	    en->entry	= bucket->entries;
	    return;
	}

	/* There was nothing in the bucket we just examined, move to the
	 * next one.
	 */
	en->bucket++;
    }

    /* Here we have exhausted all buckets and the enumeration pointer will 
     * have its bucket count = table->modulo whicih signifies that we are done.
     */
}

/** \brief Frees up the memory structures that represent a hash table
 *  enumeration.
 * \param[in] enum Pointer to ANTLR3 enumeratio structure returned by antlr3EnumNew()
 */
static void
antlr3EnumFree	(pANTLR3_HASH_ENUM en)
{
    /* Nothing to check, we just free it.
     */
    ANTLR3_FREE(en);
}

/** Given an input key of arbitrary length, return a hash value of
 *  it. This can then be used (with suitable modulo) to index other
 *  structures.
 */
ANTLR3_API ANTLR3_UINT32
antlr3Hash(void * key, ANTLR3_UINT32 keylen)
{
    /* Accumulate the hash value of the key
     */
    ANTLR3_UINT32   hash;
    pANTLR3_UINT8   keyPtr;
    ANTLR3_UINT32   i1;

    hash    = 0;
    keyPtr  = (pANTLR3_UINT8) key;

    /* Iterate the key and accumulate the hash
     */
    while(keylen > 0)
    {
	hash = (hash << 4) + (*(keyPtr++));

	if ((i1=hash&0xf0000000) != 0)
	{
		hash = hash ^ (i1 >> 24);
		hash = hash ^ i1;
	}
	keylen--;
    }

    return  hash;
}

ANTLR3_API  pANTLR3_LIST
antlr3ListNew	(ANTLR3_UINT32 sizeHint)
{
    pANTLR3_LIST    list;

    /* Allocate memory
     */
    list    = (pANTLR3_LIST)ANTLR3_MALLOC((size_t)sizeof(ANTLR3_LIST));

    if	(list == NULL)
    {
	return	(pANTLR3_LIST)ANTLR3_ERR_NOMEM;
    }

    /* Now we need to add a new table
     */
    list->table	= antlr3HashTableNew(sizeHint);

    if	(list->table == (pANTLR3_HASH_TABLE)ANTLR3_ERR_NOMEM)
    {
	return	(pANTLR3_LIST)ANTLR3_ERR_NOMEM;
    }

    /* Allocation was good, install interface
     */
    list->free	    =  antlr3ListFree;
    list->del	    =  antlr3ListDelete;
    list->get	    =  antlr3ListGet;
    list->add	    =  antlr3ListAdd;
    list->remove    =  antlr3ListRemove;
    list->put	    =  antlr3ListPut;
    list->size	    =  antlr3ListSize;

    return  list;
}

static ANTLR3_UINT64	antlr3ListSize	    (pANTLR3_LIST list)
{
    return  list->table->size(list->table);
}

static void
antlr3ListFree	(pANTLR3_LIST list)
{
    /* Free the hashtable that stores the list
     */
    list->table->free(list->table);

    /* Free the allocation for the list itself
     */
    ANTLR3_FREE(list);
}

static void
antlr3ListDelete    (pANTLR3_LIST list, ANTLR3_UINT64 key)
{
    ANTLR3_UINT8    charKey[32];

    sprintf((char *)charKey, "%lld", key);

    list->table->del(list->table, charKey);
}

static void *
antlr3ListGet	    (pANTLR3_LIST list, ANTLR3_UINT64 key)
{
    ANTLR3_UINT8    charKey[32];

    sprintf((char *)charKey, "%lld", key);

    return list->table->get(list->table, charKey);
}

/** Add the supplied element to the list, at the next available key
 */
static ANTLR3_INT32	antlr3ListAdd   (pANTLR3_LIST list, void * element, void (*freeptr)(void *))
{
    ANTLR3_UINT64   key;

    key	    = list->table->size(list->table) + 1;
    return list->put(list, key, element, freeptr);
}

/** Remove from the list, but don't free the element, just send it back to the
 *  caller.
 */
static	void *
antlr3ListRemove	    (pANTLR3_LIST list, ANTLR3_UINT64 key)
{
    pANTLR3_HASH_ENTRY	    entry;
    ANTLR3_UINT8    charKey[32];

    sprintf((char *)charKey, "%lld", key);

    entry = list->table->remove(list->table, charKey);

    if	(entry != NULL)
    {
        return  entry->data;
    }
    else
    {
	return	NULL;
    }
}

static	ANTLR3_INT32
antlr3ListPut	    (pANTLR3_LIST list, ANTLR3_UINT64 key, void * element, void (*freeptr)(void *))
{
    ANTLR3_UINT8    charKey[32];

    sprintf((char *)charKey, "%lld", key);

    return  list->table->put(list->table, (void *)charKey, element, freeptr);
}

ANTLR3_API  pANTLR3_STACK
antlr3StackNew	(ANTLR3_UINT32 sizeHint)
{
    pANTLR3_STACK   stack;

    /* Allocate memory
     */
    stack    = (pANTLR3_STACK)ANTLR3_MALLOC((size_t)sizeof(ANTLR3_STACK));

    if	(stack == NULL)
    {
	return	(pANTLR3_STACK)ANTLR3_ERR_NOMEM;
    }

    /* Now we need to add a new table
     */
    stack->list	= antlr3ListNew(sizeHint);
    stack->top	= NULL;

    if	(stack->list == (pANTLR3_LIST)ANTLR3_ERR_NOMEM)
    {
	return	(pANTLR3_STACK)ANTLR3_ERR_NOMEM;
    }

    /* Looks good, now add the interface
     */
    stack->get	=  antlr3StackGet;
    stack->free	=  antlr3StackFree;
    stack->pop	=  antlr3StackPop;
    stack->push	=  antlr3StackPush;
    stack->size	=  antlr3StackSize;
    stack->peek	=  antlr3StackPeek;

    return  stack;
}

static ANTLR3_UINT64	antlr3StackSize	    (pANTLR3_STACK stack)
{
    return  stack->list->size(stack->list);
}


static void
antlr3StackFree	(pANTLR3_STACK  stack)
{
    /* Free the list that supports the stack
     */
    stack->list->free(stack->list);
    stack->top	= NULL;

    ANTLR3_FREE(stack);
}

static void
antlr3StackPop	(pANTLR3_STACK	stack)
{
   stack->list->del(stack->list, stack->list->table->count);
   stack->top = stack->list->get(stack->list, stack->list->table->count);
}

static void *
antlr3StackGet	(pANTLR3_STACK stack, ANTLR3_UINT64 key)
{
    return  stack->list->get(stack->list, key);
}

static void *
antlr3StackPeek	(pANTLR3_STACK	stack)
{
    return  stack->top;
}

static ANTLR3_BOOLEAN 
antlr3StackPush	(pANTLR3_STACK stack, void * element, void (*freeptr)(void *))
{
    ANTLR3_UINT64	pushno;

    pushno  = stack->list->table->count + 1;

    stack->top	= element;
    return stack->list->put(stack->list, pushno, element, freeptr);
}

ANTLR3_API  pANTLR3_VECTOR
antlr3VectorNew	(ANTLR3_UINT32 sizeHint)
{
    ANTLR3_UINT32   initialSize;
    pANTLR3_VECTOR  vector;

    /* Allow vectors to be guessed by ourselves, so input size can be zero
     */
    if	(sizeHint > 0)
    {
	initialSize = sizeHint;
    }
    else
    {
	initialSize = 8;
    }

    /* Allocate memory for the vector structure itself
     */
    vector  = (pANTLR3_VECTOR) ANTLR3_MALLOC((size_t)(sizeof(ANTLR3_VECTOR)));

    if	(vector == NULL)
    {
	return	(pANTLR3_VECTOR)ANTLR3_ERR_NOMEM;
    }

    /* Now fill in the defaults
     */
    vector->elements	= (pANTLR3_VECTOR_ELEMENT)ANTLR3_MALLOC((size_t)(sizeof(ANTLR3_VECTOR_ELEMENT) * initialSize));

    if	(vector->elements == NULL)
    {
	ANTLR3_FREE(vector);
	return	(pANTLR3_VECTOR)ANTLR3_ERR_NOMEM;
    }

    /* Memory allocated succesfully
     */
    vector->count	    = 0;	    /* No entries yet of course	*/
    vector->elementsSize    = initialSize;  /* Available entries	*/

    /* Now we can install the API
     */
    vector->add	    = antlr3VectorAdd;
    vector->del	    = antlr3VectorDel;
    vector->get	    = antlr3VectorGet;
    vector->free    = antlr3VectorFree;
    vector->get	    = antlr3VectorGet;
    vector->put	    = antlr3VectorPut;
    vector->remove  = antrl3VectorRemove;
    vector->size    = antlr3VectorSize;

    /** Assume that this is not a factory made vector
     */
    vector->factoryMade	= ANTLR3_FALSE;

    /* And everything is hunky dory
     */
    return  vector;
}

static	void		antlr3VectorFree    (pANTLR3_VECTOR vector)
{
    ANTLR3_UINT64   entry;

    /* We must traverse every entry in the vector and if it has
     * a pointer to a free fucntion then we call it with the
     * the entry pointer
     */
    for	(entry = 0; entry < vector->count; entry++)
    {
	if  (vector->elements[entry].freeptr != NULL)
	{
	    vector->elements[entry].freeptr(vector->elements[entry].element);
	}
	vector->elements[entry].freeptr    = NULL;
	vector->elements[entry].element    = NULL;
    }

    if	(vector->factoryMade == ANTLR3_FALSE)
    {
	/* The entries are freed, so free the element allocation
	 */
	ANTLR3_FREE(vector->elements);
	vector->elements = NULL;

	/* Finally, free the allocation for the vector itself
	 */
	ANTLR3_FREE(vector);
    }

}

static	void		antlr3VectorDel	    (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry)
{
    /* Check this is a valid request first (index is 1 based!!)
     */
    if	(entry > vector->elementsSize || entry == 0)
    {
	return;
    }

    /* Valid request, check for free pointer and call it if present
     */
    if	(vector->elements[entry].freeptr != NULL)
    {
	vector->elements[entry].freeptr(vector->elements[entry].element);
	vector->elements[entry].freeptr    = NULL;
    }

    if	(entry == vector->count)
    {
	/* Ensure the pointer is never reused by accident, but othewise just 
	 * decrement the pointer.
	 */
	vector->elements[entry].element    = NULL;
    }
    else
    {
	/* Need to shuffle trailing pointers back over the deleted entry
	 */
	ANTLR3_MEMMOVE(vector->elements + entry - 1, vector->elements + entry, sizeof(ANTLR3_VECTOR_ELEMENT) * (vector->count - entry));
    }

    /* One less entry in the vector now
     */
    vector->count--;
}

static	void *		antlr3VectorGet     (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry)
{
    /* Ensure this is a vlaid request
     */
    if	(entry <= vector->count && entry > 0)
    {
	return	vector->elements[entry - 1].element;	/* Index is 1 based, storage is 0 based */
    }
    else
    {
	/* I know nothing, Mr. Fawlty!
	 */
	return	NULL;
    }
}

/** Remove the entry from the vector, but do not free any entry, even if it has
 * a free pointer.
 */
static	void *		antrl3VectorRemove  (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry)
{
    void * element;

    /* Check this is a valid request first (index is 1 based!!)
     */
    if	(entry > vector->elementsSize || entry == 0)
    {
	return NULL;
    }

    /* Valid request, return the sorted pointer
     */
    element				    = vector->elements[entry].element;

    if	(entry == vector->count)
    {
	/* Ensure the pointer is never reused by accident, but otherwise just 
	 * decrement the pointer.
	 */
	vector->elements[entry].element    = NULL;
	vector->elements[entry].freeptr    = NULL;
    }
    else
    {
	/* Need to shuffle trailing pointers back over the deleted entry
	 */
	ANTLR3_MEMMOVE(vector->elements + entry - 1, vector->elements + entry, sizeof(ANTLR3_VECTOR_ELEMENT) * (vector->count - entry));
    }

    /* One less entry in the vector now
     */
    vector->count--;

    return  element;
}

/* Add the supplied pointer and freeing function pointer to the list,
 * explanding the vector if needed.
 */
static	ANTLR3_INT32    antlr3VectorAdd	    (pANTLR3_VECTOR vector, void * element, void (*freeptr)(void *))
{
    /* Do we need to resize the vector table?
     */
    if	(vector->count == vector->elementsSize)
    {
	ANTLR3_UINT64	newSize;

	/* Need to resize the element pointers. We double the allocation
	 * unless we have reached 1024 elements, in which case we just
	 * add another 1024. I may tune this or make it tunable later.
	 */
	if  (vector->elementsSize > 1024)
	{
	    newSize = vector->elementsSize + 1024;
	}
	else
	{
	    newSize = vector->elementsSize * 2;
	}

	/* Use realloc so that the pointers are copied for us
	 */
	vector->elements	= (pANTLR3_VECTOR_ELEMENT)ANTLR3_REALLOC(vector->elements, (sizeof(ANTLR3_VECTOR_ELEMENT)* newSize));
	/* Reset new pointers etc to 0
	 */
	ANTLR3_MEMSET(vector->elements + vector->elementsSize, 0x00, (newSize - vector->elementsSize) * sizeof(ANTLR3_VECTOR_ELEMENT));
	vector->elementsSize	= newSize;
    }

    /* Insert the new entry
     */
    vector->elements[vector->count].element	= element;
    vector->elements[vector->count].freeptr	= freeptr;

    vector->count++;	    /* One more element counted	*/

    return  (ANTLR3_UINT32)(vector->count);

}

/* Replace the element at the specified entry point with the supplied
 * entry.
 */
static	ANTLR3_INT32    antlr3VectorPut	    (pANTLR3_VECTOR vector, ANTLR3_UINT64 entry, void * element, void (*freeptr)(void *))
{
    /* Validate first
     */
    if	(entry > vector->count || entry == 0)
    {
	return	-1;
    }

    /* Valid request, replace teh current one
     */
    if	(vector->elements[entry].freeptr != NULL)
    {
	vector->elements[entry].freeptr(vector->elements[entry].element);
    }

    /* Install the new pointers
     */
    vector->elements[entry].freeptr    = freeptr;
    vector->elements[entry].element	= element;

    return  (ANTLR3_UINT32)(entry);	    /* Indicates the replacement was successful	*/

}

static	ANTLR3_UINT64   antlr3VectorSize    (pANTLR3_VECTOR vector)
{
    return  vector->count;
}

/** Vector factory creation
 */
ANTLR3_API pANTLR3_VECTOR_FACTORY   antlr3VectorFactoryNew	    (ANTLR3_UINT32 sizeHint)
{
    pANTLR3_VECTOR_FACTORY  factory;

    /* Allocate memory for the factory
     */
    factory = (pANTLR3_VECTOR_FACTORY)ANTLR3_MALLOC((size_t)(sizeof(ANTLR3_VECTOR_FACTORY)));

    if	(factory == NULL)
    {
	return	(pANTLR3_VECTOR_FACTORY)ANTLR3_ERR_NOMEM;
    }

    /* Factory memory is good, so create a new vector
     */
    if	(sizeHint == 0)
    {
	sizeHint = 64;
    }

    /* Create the vector if possible
     */
    factory->vectors	= antlr3VectorNew(sizeHint);

    if	(factory->vectors == (pANTLR3_VECTOR)ANTLR3_ERR_NOMEM)
    {
	ANTLR3_FREE(factory);
	return	(pANTLR3_VECTOR_FACTORY)ANTLR3_ERR_NOMEM;
    }

    /* Install the API
     */
    factory->close	= closeVectorFactory;
    factory->newVector	= newVector;

    return  factory;
}

static  void		
closeVectorFactory  (pANTLR3_VECTOR_FACTORY factory)
{
    ANTLR3_UINT64   entry;
    pANTLR3_VECTOR  vector;
    pANTLR3_VECTOR  freeVector;

    /** First we iterate the vectors in the factory and call
     *  free on each of them. Because they are factory made only
     *  any installed free pointers for the entries will be called
     *  and we will be left with just those vectors that were factory made
     *  to delete the memory allocations for. These are the elment list
     *  itself.
     */
    vector  = factory->vectors;

    /* We must traverse every entry in the vector and if it has
     * a pointer to a free function then we call it with the
     * the entry pointer
     */
    for	(entry = 0; entry < vector->count; entry++)
    {
	if  (vector->elements[entry].freeptr != NULL)
	{
	    vector->elements[entry].freeptr(vector->elements[entry].element);
	}
    }

    /* Having called free on each of the vectors in the vector factory,
     * anything that was somewhere buried in the vectors in the factory that
     * was not factory made, is now deallocated. So, we now need only
     * traverse each vector in the factory and free its elements, then free this
     * factory vector.
     */
    for	(entry = 0; entry < vector->count; entry++)
    {
	freeVector  = (pANTLR3_VECTOR)(vector->elements[entry].element);

	/* Anything in here should be factory made, but we do this just
	 * to triple check.
	 */
	if  (freeVector->factoryMade == ANTLR3_TRUE)
	{
	    ANTLR3_FREE(freeVector->elements);
	    ANTLR3_FREE(freeVector);
	}
    }

    /* Free the memory for the factory vector elements, then the factory vector
     */
    ANTLR3_FREE(vector->elements);
    ANTLR3_FREE(vector);

    /* Now free the memory for the factory itself
     */
    ANTLR3_FREE(factory);
}

static	pANTLR3_VECTOR  
newVector	    (pANTLR3_VECTOR_FACTORY factory)
{
    pANTLR3_VECTOR  vector;

    /* Attempt to create a new vector of default size
     */
    vector  = antlr3VectorNew(0);

    if	(vector == (pANTLR3_VECTOR)(ANTLR3_ERR_NOMEM))
    {
	return vector;
    }

    /* Now add this vector to the factory vector, which will
     * track it and release any entries in it when we close the factory.
     */
    vector->factoryMade	= ANTLR3_TRUE;
    factory->vectors->add(factory->vectors, (void *)vector, (void (*)(void *))(vector->free));

    return  vector;
}
