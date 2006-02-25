/** \file
 * Declarations for all the antlr3 C runtime intrfaces/classes. This
 * allows the structures that define th einterfaces to contain pointers to
 * each other without trying to sort out the cyclic inter-dependancies that
 * would otherwise result.
 */
#ifndef	_ANTLR3_INTERFACES_H
#define	_ANTLR3_INTERFACES_H

typedef	struct ANTLR3_INT_STREAM_struct		*pANTLR3_INT_STREAM;

typedef struct ANTLR3_BASE_RECOGNIZER_struct	*pANTLR3_BASE_RECOGNIZER;

typedef struct ANTLR3_BITSET_struct		*pANTLR3_BITSET;

typedef struct ANTLR3_TOKEN_FACTORY_struct	*pANTLR3_TOKEN_FACTORY;
typedef struct ANTLR3_COMMON_TOKEN_struct	*pANTLR3_COMMON_TOKEN;

typedef struct ANTLR3_EXCEPTION_struct		*pANTLR3_EXCEPTION;

typedef struct ANTLR3_HASH_BUCKET_struct	*pANTLR3_HASH_BUCKET;
typedef struct ANTLR3_HASH_ENTRY_struct		*pANTLR3_HASH_ENTRY;
typedef struct ANTLR3_HASH_ENUM_struct		*pANTLR3_HASH_ENUM;
typedef struct ANTLR3_HASH_TABLE_struct		*pANTLR3_HASH_TABLE;

typedef struct ANTLR3_LIST_struct		*pANTLR3_LIST;
typedef struct ANTLR3_STACK_struct		*pANTLR3_STACK;

typedef struct ANTLR3_INPUT_STREAM_struct	*pANTLR3_INPUT_STREAM;
typedef struct ANTLR3_LEX_STATE_struct		*pANTLR3_LEX_STATE;

typedef struct ANTLR3_STRING_FACTORY_struct	*pANTLR3_STRING_FACTORY;
typedef struct ANTLR3_STRING_struct		*pANTLR3_STRING;

typedef struct ANTLR3_TOKEN_SOURCE_struct	    *pANTLR3_TOKEN_SOURCE;
typedef	struct ANTLR3_TOKEN_STREAM_struct	    *pANTLR3_TOKEN_STREAM;
typedef	struct ANTLR3_COMMON_TOKEN_STREAM_struct    *pANTLR3_COMMON_TOKEN_STREAM;

typedef	struct ANTLR3_DFA_STATE_struct		*pANTLR3_DFA_STATE;
typedef	struct ANTLR3_DFA_struct		*pANTLR3_DFA;
#endif