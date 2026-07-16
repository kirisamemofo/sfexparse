/*
 * sfexparse - Single file C89 expression parser and resolver library
 *
 * Copyright (c) 2026 Devon Artmeier
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Define to use floating point numbers */
/* #define SFEXPARSE_USE_FLOAT */

/*
 * HOW TO USE:
 * 
 * To incorporate the implementation of this library, do this:
 * 
 *     #define SFEXPARSE_IMPLEMENTATION
 *     #include "sfexpare.h"
 * 
 * When working with this library, if an error occurs, then an error code will be
 * set, which can be retrieved by calling sfe_get_error(), or sfe_error_string()
 * to get a string representation of the error code. If there is no error, then
 * SFE_OK will be set as the error code:
 * 
 *     if (sfe_get_error() != SFE_OK) {
 *         printf("Error: %s\n", sfe_error_string());
 *     }
 * 
 * In order to utilize variables and functions during resolution, value getter and
 * setter functions need to be defined, alongside a symbol validator function, and
 * then passed into sfe_init():
 * 
 *     sfe_init(getter, setter, validator);
 * 
 * Said functions have the following signatures:
 * 
 *     sfe_error getter(const sfe_node* const node, sfe_value* const value)
 *     sfe_error setter(const sfe_node* const node, const sfe_value* const value)
 *     sfe_bool validator(const char* symbol);
 * 
 * When either the getter or setter are called, it will be given a pointer to a graph
 * node which includes things such as the symbol name, list of arguments for function,
 * calls, and list of subscript values for arrays:
 * 
 *     node->type          - Type          (SFE_NUMBER, SFE_STRING, SFE_SYMBOL,
 *                                          SFE_FUNCTION, or SFE_ARRAY)
 *     node->number        - Number        (if type is SFE_NUMBER)
 *     node->string        - String/Symbol (if type is SFE_STRING, SFE_SYMBOL,
 *                                          SFE_FUNCTION, or SFE_ARRAY)
 *     node->subs          - Function arguments/array subscripts
 *     node->sub_count     - Function argument/array subscript count
 *     node->subs->head[i] - Function argument/array subscript node
 * 
 * The getter function will be given a pointer in which the retrieved value info
 * should be stored into. Use sfe_set_number() if the value is a number, or use
 * sfe_set_string() if the value is a string (which returns SFE_TRUE or SFE_FALSE
 * depending on if it succeeded at setting the string or not):
 * 
 *     if (number) {
 *         sfe_set_number(value, number);
 *     } else {
 *         if (!sfe_set_string(value, string)) {
 *             [Failed to set string value]
 *         }
 *     }
 * 
 * If successful, SFE_OK should be returned. If not, SFE_CANT_GET_VALUE should be
 * returned instead:
 *
 *     if (failed) {
 *         return SFE_CANT_GET_VALUE;
 *     }
 *     return SFE_OK;
 * 
 * If there's a problem with the number of function arguments or subscripts
 * passed, then either SFE_NOT_ENOUGH_ARGUMENTS or SFE_TOO_MANY_ARGUMENTS should
 * be returned:
 * 
 *     if (count < [expected count]) {
 *         return SFE_NOT_ENOUGH_ARGUMENTS;
 *     } else if (count > [expected count]) {
 *         return SFE_TOO_MANY_ARGUMENTS;
 *     }
 * 
 * If there's a problem retrieving a function argument or array subscript, then
 * SFE_BAD_ARGUMENT or SFE_BAD_SUBSCRIPT should be returned:
 * 
 *     if (type == SFE_FUNCTION) {
 *         return SFE_BAD_ARGUMENT;
 *     } else {
 *         return SFE_BAD_SUBSCRIPT;
 *     }
 * 
 * The setter function will be given the value info in which to set with:
 * 
 *     value->type   - Type   (SFE_NUMBER or SFE_STRING)
 *     value->number - Number (if type is SFE_NUMBER)
 *     value->string - String (if type is SFE_STRING)
 * 
 * If successful, SFE_OK should be returned. If not, SFE_CANT_SET_VALUE should be
 * returned instead:
 * 
 *     if (failed) {
 *         return SFE_CANT_SET_VALUE;
 *     }
 *     return SFE_OK;
 * 
 * The symbol validator function will be given a symbol to check. If it's valid,
 * SFE_TRUE should be returned. If not, SFE_FALSE should be returned instead:
 *
 *    if (!valid) {
 *        return SFE_FALSE;
 *    }
 *    return SFE_TRUE;
 *
 * To parse an expression, it's as simple as calling sfe_parse() with the
 * expression string passed to it. If there's a syntax error, a null pointer
 * will be returned, and an error code will be set, If the expression is empty,
 * then a null pointer will be returned, but with no error code. Otherwise,
 * it will return a graph generated from the given expression, where numbers and
 * subexpressions are optimized:
 * 
 *     sfe_graph* graph = sfe_parse("(2 + 2)");
 *     if (graph) {
 *         ["graph" is valid here, and represents "4" after optimization]
 *     } else if (sfe_get_error() != SFE_OK) {
 *         ["graph" is null here, with an error code set]
 *     }
 * 
 * To get a string representation of the graph, just call sfe_to_string(). If
 * it fails to do that, then a null pointer will be returned with an error
 * code set. When finished using the string, don't forget to call free() on it
 * to prevent a memory leak:
 * 
 *     char* string = sfe_to_string(graph);
 *     if (string) {
 *         ["string" is valid here]
 *         free(string)
 *     } else {
 *         ["string" is null here, with an error code set]
 *     }
 * 
 * Afterwards, the graph can be resolved into a single node by passing the
 * graph into sfe_resolve() along with a pointer to where the result should
 * be stored. When called, the chosen value getter and setter* will be utilized
 * to help resolve variables and functions. If it fails to  resolve the graph,
 * then SFE_FALSE will be returned with an error code set. Otherwise, the result
 * is returned. If the result is a string, make sure to call free() on it when
 * finished to prevent a memory leak:
 * 
 *     sfe_value result;
 *     sfe_bool success = sfe_resolve(graph, &result);
 *     if (success) {
 *         ["result" is valid here]
 *         if (result->string) {
 *             free(result->string);
 *         }
 *     } else {
 *         ["result" is invalid here, with an error code set]
 *     }
 * 
 * When finished with the graph, it should be deleted via sfe_delete() to
 * prevent a memory leak:
 * 
 *     if (graph) {
 *         sfe_delete(graph);
 *     }
 */

/*
 * SYNTAX RULES:
 * 
 * Numbers:
 * 
 *     Any digit from 0 to 9 is valid. If floating point numbers are enabled,
 *     then a decimal can be placed:
 * 
 *         0.5
 *         2.25
 * 
 *     Multiple decimal points cannot be placed, however:
 * 
 *         0..5
 *         2.2.5
 * 
 *     Integers can have a prefix that denotes what base it represents:
 * 
 *         0b1010 (Binary)
 *         0o724  (Octal)
 *         0xF8A3 (Hexadecimal)
 * 
 *     Binary numbers can only have 0s or 1s. Octal numbers can only have
 *     digits from 0 to 7. Hexadecimal numbers adds digits A to F.
 * 
 *     Nodes with a number are set to type SFE_NUMBER.
 * 
 * Strings:
 * 
 *     If there's a string of characters surrounded by a ' or ", then it will
 *     be considered a string.
 * 
 *     Nodes with a string are set to type SFE_STRING.
 * 
 * Symbols:
 * 
 *     If a token isn't prefixed with a number and contains non-numerical
 *     characters, then it will be considered a symbol. Symbols can be used
 *     to represent variables, functions, and arrays.
 * 
 *     Nodes with a symbol are set to type SFE_SYMBOL.
 * 
 * Subexpressions:
 * 
 *     Surrounding anything with a pair of parentheses will be recognized
 *     as a subexpression, which has the highest precedence over any
 *     operator. Useful for controlling the order of operations:
 * 
 *         (x + y)
 *         ((x + y) - z)
 *         ((((x))) + ((y))) - z
 * 
 *     Nodes with a subexpression are set to type SFE_SUBEXPRESSION.
 * 
 * Functions:
 * 
 *     Any symbol with a pair of parentheses after will be recognized as a
 *     function. Functions can have any number of arguments, which are
 *     separated by commas:
 *
 *         function()
 *         function(x)
 *         function(x, y, z)
 * 
 *     Nodes with function arguments are set to type SFE_FUNCTION.
 * 
 * Arrays:
 * 
 *     Any symbol with one or more pairs of brackets after will be recognized
 *     as an array with subscripts indexing it:
 * 
 *         array[x]
 *         array[x][y][z]
 * 
 *     Nodes with array subscripts are set to type SFE_ARRAY.
 * 
 * Unary operators:
 * 
 *     + (Positive)
 *     - (Negative)
 *     ~ (Bitwise NOT)
 *     ! (Logical NOT)
 * 
 *     One or more of them can be placed before any operand (except strings):
 * 
 *         -25
 *         ~+x
 *         ~~(x + y)
 *         !function()
 *         !~-+array[x]
 * 
 *     To avoid a chain of positive or negative operators from being detected
 *     as an increment, either a space must be placed inbetween, or a
 *     subexpression must separate them:
 *     
 *         + +x
 *         -(-x)
 * 
 * Increment operators:
 * 
 *     ++ (Increment)
 *     -- (Decrement)
 * 
 *     They can be placed before or after a symbol or array operand containing
 *     a number (not a string):
 * 
 *         ++x
 *         array[x]--
 * 
 *     They cannot be placed with a number, string, subexpression, or function:
 * 
 *         ++2
 *         "string"--
 *         --(x)
 *         (x + y)++
 *         function(x)--
 * 
 *     They cannot be placed before a unary operator:
 * 
 *         ++-x
 *         --!array[x]
 * 
 *     A pre-increment operator and post-increment operator cannot co-exist:
 * 
 *         ++x++
 *         --array[x]--
 * 
 *     To distinguish between an increment operator from a positive or negative
 *     operator, a space must be placed inbetween:
 * 
 *         + ++x
 *         - --array[x]
 * 
 * Binary operators:
 * 
 *     +  (Addition)
 *     -  (Subtraction)
 *     *  (Multiplication)
 *     ** (Exponentiation)
 *     /  (Division)
 *     %  (Modulo)
 *     == (Equal)
 *     != (Not equal)
 *     <  (Less than)
 *     <= (Less than or equal)
 *     >  (Greater than)
 *     >= (Greater than or equal)
 *     && (Logical AND)
 *     || (Logical OR)
 *     &  (Bitwise AND)
 *     |  (Bitwise OR)
 *     ^  (Bitwise XOR)
 *     << (Left shift)
 *     >> (Right shift)
 * 
 *     They MUST be placed between 2 operands:
 * 
 *         x * y
 * 
 *     When working with strings, only the addition and comparison operators
 *     can be used:
 * 
 *         "x" + "y"
 *         "x" == "y"
 *         "x" != "y"
 *         "x" < "y"
 *         "x" <= "y"
 *         "x" > "y"
 *         "x" >= "y"
 * 
 *     When working with both numbers and strings, only the addition operator
 *     can be used to combine them:
 *     
 *         "x" + 1 (appends "1" to "x")
 * 
 * Assignment operators:
 * 
 *     =   (Assignment)
 *     +=  (Addition)
 *     -=  (Subtraction)
 *     *=  (Multiplication)
 *     **= (Exponentiation)
 *     /=  (Division)
 *     %=  (Modulo)
 *     &=  (Bitwise AND)
 *     |=  (Bitwise OR)
 *     ^=  (Bitwise XOR)
 *     <<= (Left shift)
 *     >>= (Right shift)
 * 
 *     Assignments can be done on variables and arrays that DON'T have any
 *     unary or increment operators on them:
 * 
 *         x = y
 *         array[x] = y
 * 
 *     Some binary operators can become assignment operators, so that
 *     variables and arrays can have arithmetic directly applied to
 *     them:
 *     
 *         x += y
 *         x *= (y - z)
 * 
 *     When working with strings, only regular and addition assignment
 *     operators can be used:
 * 
 *         x = "y"
 *         x += "y"
 * 
 *     When working with both numbers and strings, only the addition
 *     assignment operator can be used to combine them:
 *     
 *         x += 1 (appends "1" to the string stored in x)
 */

/*
 * ERROR TYPES
 * 
 *     SFE_OK                   (No error)
 *     SFE_ALLOCATION_FAILED    (Allocation failed)
 *     SFE_BAD_NODE             (Invalid node)
 *     SFE_NO_OPERATOR          (No operator)
 *     SFE_BAD_OPERATOR         (Invalid operator)
 *     SFE_NO_OPERAND           (No operand)
 *     SFE_NO_LEFT_OPERAND      (No left operand)
 *     SFE_NO_RIGHT_OPERAND     (No right operand)
 *     SFE_NO_SUBEXPRESSION     (No subexpression)
 *     SFE_NO_ARGUMENT          (No argument)
 *     SFE_NO_SUBSCRIPT         (No subscript)
 *     SFE_BAD_SUBSCRIPT        (Invalid subscript)
 *     SFE_NO_LEFT_PARENTHESIS  (No left parenthesis)
 *     SFE_NO_RIGHT_PARENTHESIS (No right parenthesis)
 *     SFE_NO_LEFT_BRACKET      (No left bracket)
 *     SFE_NO_RIGHT_BRACKET     (No right bracket)
 *     SFE_NO_RIGHT_QUOTE       (No right quotation mark)
 *     SFE_NO_ARRAY_NAME        (No array name)
 *     SFE_UNEXPECTED_COMMA     (Unexpected comma)
 *     SFE_NOT_NUMBER           (Not a number)
 *     SFE_BAD_NUMBER           (Invalid number)
 *     SFE_BAD_SYMBOL           (Invalid symbol)
 *     SFE_DIVIDE_BY_ZERO       (Division by 0)
 *     SFE_CANT_GET_VALUE       (Can't get value)
 *     SFE_CANT_SET_VALUE       (Can't set value)
 *     SFE_NOT_ENOUGH_ARGUMENTS (Not enough arguments)
 *     SFE_TOO_MANY_ARGUMENTS   (Too many arguments)
 *     SFE_BAD_ARGUMENT         (Invalid argument)
 */

/*
 * CHANGELOG:
 *     v1.0   (2026/07/10) - Initial version
 *     v1.0.1 (2026/07/14) - Add symbol validator support
 *     v1.1   (2026/07/15) - Add string support
 */

#ifndef SFEXPARSE_H
#define SFEXPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#define SFE_NULL (0)
#define SFE_TRUE (1)
#define SFE_FALSE (0)

typedef char sfe_bool;
typedef long sfe_int;
typedef unsigned long sfe_uint;
#ifndef SFEXPARSE_USE_FLOAT
typedef long sfe_num;
#else
typedef double sfe_num;
#endif

typedef enum {
    SFE_OK = 0,
    SFE_ALLOCATION_FAILED,
    SFE_BAD_NODE,
    SFE_NO_OPERATOR,
    SFE_BAD_OPERATOR,
    SFE_NO_OPERAND,
    SFE_NO_LEFT_OPERAND,
    SFE_NO_RIGHT_OPERAND,
    SFE_NO_SUBEXPRESSION,
    SFE_NO_ARGUMENT,
    SFE_NO_SUBSCRIPT,
    SFE_BAD_SUBSCRIPT,
    SFE_NO_LEFT_PARENTHESIS,
    SFE_NO_RIGHT_PARENTHESIS,
    SFE_NO_LEFT_BRACKET,
    SFE_NO_RIGHT_BRACKET,
    SFE_NO_RIGHT_QUOTE,
    SFE_NO_ARRAY_NAME,
    SFE_UNEXPECTED_COMMA,
    SFE_NOT_NUMBER,
    SFE_BAD_NUMBER,
    SFE_BAD_SYMBOL,
    SFE_DIVIDE_BY_ZERO,
    SFE_CANT_GET_VALUE,
    SFE_CANT_SET_VALUE,
    SFE_NOT_ENOUGH_ARGUMENTS,
    SFE_TOO_MANY_ARGUMENTS,
    SFE_BAD_ARGUMENT,
    SFE_ERROR_COUNT
} sfe_error;

typedef enum {
    SFE_TYPE_UNKNOWN = -1,
    SFE_NUMBER,
    SFE_STRING,
    SFE_SYMBOL,
    SFE_SUBEXPRESSION,
    SFE_FUNCTION,
    SFE_ARRAY,
    SFE_TYPE_COUNT
} sfe_type;

typedef enum {
    SFE_POSITIVE = 0,
    SFE_NEGATIVE,
    SFE_LOGICAL_NOT,
    SFE_BITWISE_NOT,
    SFE_UNARY_COUNT
} sfe_unary;

typedef enum {
    SFE_INCREMENT = 0,
    SFE_DECREMENT,
    SFE_INCREMENT_COUNT
} sfe_inc;

typedef enum {
    SFE_BINARY_UNKNOWN = -1,
    SFE_ADD,
    SFE_SUBTRACT,
    SFE_MULTIPLY,
    SFE_EXPONENT,
    SFE_DIVIDE,
    SFE_MODULO,
    SFE_EQUAL,
    SFE_NOT_EQUAL,
    SFE_LESS,
    SFE_LESS_EQUAL,
    SFE_GREATER,
    SFE_GREATER_EQUAL,
    SFE_LOGICAL_AND,
    SFE_LOGICAL_OR,
    SFE_BITWISE_AND,
    SFE_BITWISE_OR,
    SFE_BITWISE_XOR,
    SFE_SHIFT_LEFT,
    SFE_SHIFT_RIGHT,
    SFE_BINARY_COUNT
} sfe_binary;

struct sfe_node_;

typedef struct sfe_graph_ {
    struct sfe_node_* head;
    struct sfe_node_* tail;
    struct sfe_node_* parent;
} sfe_graph;

typedef struct sfe_node_ {
    sfe_type type;
    sfe_num number;
    char* string;
    sfe_unary* unary;
    sfe_inc* pre;
    sfe_inc* post;
    sfe_uint unary_count;
    sfe_uint pre_count;
    sfe_uint post_count;
    sfe_binary binary;
    sfe_bool assign;
    sfe_graph* graph;
    sfe_graph** subs;
    sfe_uint sub_count;
    struct sfe_node_* previous;
    struct sfe_node_* next;
} sfe_node;

typedef struct sfe_value_ {
    sfe_type type;
    sfe_num number;
    char* string;
} sfe_value;

typedef sfe_error (*sfe_getter)(const sfe_node* const node, sfe_value* const value);
typedef sfe_error (*sfe_setter)(const sfe_node* const node, const sfe_value* const value);
typedef sfe_bool (*sfe_validator)(const char* symbol);

/*
 * Initialize
 *
 * ARGUMENTS:
 *     getter    - Value getter function
 *     setter    - Value setter function
 *     validator - Symbol validator function
 * RETURNS:
 *     Error code
 */
void sfe_init(sfe_getter getter, sfe_setter setter, sfe_validator validator);

/*
 * Get error code
 *
 * RETURNS:
 *     Error code
 */
sfe_error sfe_get_error(void);

/*
 * Get string representation of error code
 *
 * RETURNS:
 *     Error code string
 */
const char* sfe_error_string(void);

/*
 * Set value number
 *
 * ARGUMENTS:
 *     value  - Value to store number in
 *     number - Number to store
 */
void sfe_set_number(sfe_value* const node, sfe_num number);

/*
 * Set value string
 *
 * ARGUMENTS:
 *     value  - Value to store string in
 *     string - String to store
 * RETURNS:
 *     True if successful, false if failed
 */
sfe_bool sfe_set_string(sfe_value* const node, char* string);

/*
 * Parse expression
 *
 * ARGUMENTS:
 *     expression - Expression
 * RETURNS:
 *     Graph if successful, null if failed
 */
sfe_graph* sfe_parse(const char* expression);

/*
 * Clone graph
 *
 * ARGUMENTS:
 *     graph - Graph
 * RETURNS:
 *     Cloned graph if successful, null if failed
 */
sfe_graph* sfe_clone(sfe_graph* graph);

/*
 * Convert graph to string
 *
 * ARGUMENTS:
 *     graph - Graph
 * RETURNS:
 *     String if successful, null if failed
 */
char* sfe_to_string(sfe_graph* graph);

/*
 * Delete graph
 *
 * ARGUMENTS:
 *     graph - Graph
 */
void sfe_delete(sfe_graph* graph);

/*
 * Resolve graph
 *
 * ARGUMENTS:
 *     graph  - Graph
 *     result - Result storage
 * RETURNS:
 *     True if successful, false if failed
 */
sfe_bool sfe_resolve(sfe_graph* graph, sfe_value* result);

#ifdef SFEXPARSE_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    SFE_GROUP_ASSIGN = 0,
    SFE_GROUP_MULTIPLY = SFE_GROUP_ASSIGN,
    SFE_GROUP_ADD,
    SFE_GROUP_SHIFT,
    SFE_GROUP_COMPARE,
    SFE_GROUP_EQUALITY,
    SFE_GROUP_BITWISE_AND,
    SFE_GROUP_BITWISE_OR,
    SFE_GROUP_BITWISE_XOR,
    SFE_GROUP_LOGICAL_AND,
    SFE_GROUP_LOGICAL_OR,
    SFE_GROUP_COUNT
} sfe_group_id;

typedef struct sfe_work_ {
    const char* expression;
    const char* string_start;
    const char* string_end;
    const char* symbol_start;
    const char* symbol_end;
} sfe_work;

typedef struct {
    sfe_node** array;
    sfe_uint count;
} sfe_group;

static sfe_error sfe_error_code = SFE_OK;
static sfe_getter sfe_do_get_value = SFE_NULL;
static sfe_setter sfe_do_set_value = SFE_NULL;
static sfe_validator sfe_do_validate = SFE_NULL;

static sfe_group_id sfe_group_ids[SFE_BINARY_COUNT] = {
    SFE_GROUP_ADD,              /* SFE_ADD */
    SFE_GROUP_ADD,              /* SFE_SUBTRACT */
    SFE_GROUP_MULTIPLY,         /* SFE_MULTIPLY */
    SFE_GROUP_MULTIPLY,         /* SFE_EXPONENT */
    SFE_GROUP_MULTIPLY,         /* SFE_DIVIDE */
    SFE_GROUP_MULTIPLY,         /* SFE_MODULO */
    SFE_GROUP_EQUALITY,         /* SFE_EQUAL */
    SFE_GROUP_EQUALITY,         /* SFE_NOT_EQUAL */
    SFE_GROUP_COMPARE,          /* SFE_LESS */
    SFE_GROUP_COMPARE,          /* SFE_LESS_EQUAL */
    SFE_GROUP_COMPARE,          /* SFE_GREATER */
    SFE_GROUP_COMPARE,          /* SFE_GREATER_EQUAL */
    SFE_GROUP_LOGICAL_AND,      /* SFE_LOGICAL_AND */
    SFE_GROUP_LOGICAL_OR,       /* SFE_LOGICAL_OR */
    SFE_GROUP_BITWISE_AND,      /* SFE_BITWISE_AND */
    SFE_GROUP_BITWISE_OR,       /* SFE_BITWISE_OR */
    SFE_GROUP_BITWISE_XOR,      /* SFE_BITWISE_XOR */
    SFE_GROUP_SHIFT,            /* SFE_SHIFT_LEFT */
    SFE_GROUP_SHIFT             /* SFE_SHIFT_RIGHT */
};

void sfe_init(sfe_getter getter, sfe_setter setter, sfe_validator validator)
{
    sfe_do_get_value = getter;
    sfe_do_set_value = setter;
    sfe_do_validate = validator;
}

sfe_error sfe_get_error(void)
{
    return sfe_error_code;
}

const char* sfe_error_string(void)
{
    static const char* errors[SFE_ERROR_COUNT] = {
        "OK",                           /* SFE_OK */
        "Allocation failed",            /* SFE_ALLOCATION_FAILED */
        "Invalid node",                 /* SFE_BAD_NODE */
        "No operator",                  /* SFE_NO_OPERATOR */
        "Invalid operator",             /* SFE_BAD_OPERATOR */
        "No operand",                   /* SFE_NO_OPERAND */
        "No left operand",              /* SFE_NO_LEFT_OPERAND */
        "No right operand",             /* SFE_NO_RIGHT_OPERAND */
        "No subexpression",             /* SFE_NO_SUBEXPRESSION */
        "No argument",                  /* SFE_NO_ARGUMENT */
        "No subscript",                 /* SFE_NO_SUBSCRIPT */
        "Invalid subscript",            /* SFE_BAD_SUBSCRIPT */
        "No left parenthesis",          /* SFE_NO_LEFT_PARENTHESIS */
        "No right parenthesis",         /* SFE_NO_RIGHT_PARENTHESIS */
        "No left bracket",              /* SFE_NO_LEFT_BRACKET */
        "No right bracket",             /* SFE_NO_RIGHT_BRACKET */
        "No right quotation mark",      /* SFE_NO_RIGHT_QUOTE */
        "No array name",                /* SFE_NO_ARRAY_NAME */
        "Unexpected comma",             /* SFE_UNEXPECTED_COMMA */
        "Not a number",                 /* SFE_NOT_NUMBER */
        "Invalid number",               /* SFE_BAD_NUMBER */
        "Invalid symbol",               /* SFE_BAD_SYMBOL */
        "Division by 0",                /* SFE_DIVIDE_BY_ZERO */
        "Can't get value",              /* SFE_CANT_GET_VALUE */
        "Can't set value",              /* SFE_CANT_SET_VALUE */
        "Not enough arguments",         /* SFE_NOT_ENOUGH_ARGUMENTS */
        "Too many arguments",           /* SFE_TOO_MANY_ARGUMENTS */
        "Invalid argument"              /* SFE_BAD_ARGUMENT */
    };

    return errors[sfe_error_code];
}

static sfe_node* sfe_create_node(void)
{
    sfe_node* node;

    node = (sfe_node*)malloc(sizeof(sfe_node));
    if (!node) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_NULL;
    }
    memset(node, 0, sizeof(sfe_node));

    node->type = SFE_TYPE_UNKNOWN;
    node->binary = SFE_BINARY_UNKNOWN;

    sfe_error_code = SFE_OK;
    return node;
}

static void sfe_delete_subs(sfe_node* node)
{
    sfe_uint i;

    for (i = 0; i < node->sub_count; i++) {
        sfe_delete(node->subs[i]);
    }
    if (node->subs) {
        free(node->subs);
        node->subs = SFE_NULL;
    }
    node->sub_count = 0;
}

static void sfe_delete_node(sfe_node* node)
{
    if (node->string) {
        free(node->string);
    }

    if (node->unary) {
        free(node->unary);
    }
    if (node->pre) {
        free(node->pre);
    }
    if (node->post) {
        free(node->post);
    }

    if (node->graph) {
        if (node->graph->head == node) {
            node->graph->head = node->next;
        }
        if (node->graph->tail == node) {
            node->graph->tail = node->previous;
        }
    }
    sfe_delete_subs(node);

    if (node->previous) {
        if (node->previous->next == node) {
            node->previous->next = node->next;
        }
    }
    if (node->next) {
        if (node->next->previous == node) {
            node->next->previous = node->previous;
        }
    }
    
    free(node);
}

static char* sfe_clone_string(char* string)
{
    char* clone = SFE_NULL;
    size_t length;

    if (string) {
        length = (strlen(string) + 1) * sizeof(char);
        clone = malloc(length);
        if (!clone) {
            sfe_error_code = SFE_ALLOCATION_FAILED;
            return SFE_NULL;
        }
        memcpy(clone, string, length);
    }

    sfe_error_code = SFE_OK;
    return clone;
}

void sfe_set_number(sfe_value* const value, sfe_num number)
{
    value->type = SFE_NUMBER;
    value->number = number;

    if (value->string) {
        free(value->string);
        value->string = SFE_NULL;
    }
}

sfe_bool sfe_set_string(sfe_value* const value, char* string)
{
    value->type = SFE_STRING;
    value->number = 0;

    if (value->string) {
        free(value->string);
    }
    value->string = sfe_clone_string(string);
    if (sfe_error_code != SFE_OK) {
        return SFE_FALSE;
    }

    return SFE_TRUE;
}

static void sfe_add_node(sfe_graph* graph, sfe_node* node)
{
    node->graph = graph;

    if (!graph->head) {
        graph->head = node;
    }
    if (graph->tail) {
        graph->tail->next = node;
        node->previous = graph->tail;
    }
    graph->tail = node;
}

static sfe_graph* sfe_create_graph(sfe_node* parent, sfe_bool create)
{
    sfe_graph* graph;
    sfe_node* node;

    graph = (sfe_graph*)malloc(sizeof(sfe_graph));
    if (!graph) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_NULL;
    }
    memset(graph, 0, sizeof(sfe_graph));

    graph->parent = parent;

    if (create) {
        node = sfe_create_node();
        if (!node) {
            sfe_delete(graph);
            return SFE_NULL;
        }
        sfe_add_node(graph, node);
    }

    sfe_error_code = SFE_OK;
    return graph;
}

void sfe_delete(sfe_graph* graph)
{
    sfe_node* node;
    sfe_node* next;

    if (graph) {
        node = graph->head;
        while (node) {
            next = node->next;
            sfe_delete_node(node);
            node = next;
        }
        free(graph);
    }
}

static sfe_bool sfe_check_empty(sfe_graph* graph)
{
    if (!graph) {
        return SFE_TRUE;
    } else if (!graph->head) {
        return SFE_TRUE;
    } else if (!graph->head->next && graph->head->type == SFE_TYPE_UNKNOWN) {
        return SFE_TRUE;
    }

    return SFE_FALSE;
}

static sfe_graph* sfe_create_sub(sfe_node* node)
{
    sfe_graph** array;
    sfe_graph* sub;

    if (node->type != SFE_ARRAY && node->pre) {
        sfe_error_code = SFE_BAD_OPERATOR;
        return SFE_NULL;
    }

    array = (sfe_graph**)realloc(node->subs, ((size_t)node->sub_count + 1) * sizeof(sfe_graph*));
    if (!array) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_NULL;
    }
    node->subs = array;

    sub = sfe_create_graph(node, SFE_TRUE);
    if (!sub) {
        return SFE_NULL;
    }
    node->subs[node->sub_count++] = sub;

    sfe_error_code = SFE_OK;
    return sub;
}

static sfe_graph* sfe_do_clone(sfe_graph* graph, sfe_node* parent, sfe_bool increment)
{
    sfe_graph* graph_clone;
    sfe_node* node_clone;
    sfe_node* node;
    sfe_uint i;
    size_t length;

    graph_clone = sfe_create_graph(parent, SFE_FALSE);
    if (!graph_clone) {
        return SFE_FALSE;
    }

    node = graph->head;
    while (node) {
        node_clone = sfe_create_node();
        if (!node_clone) {
            sfe_delete(graph_clone);
            return SFE_FALSE;
        }
        sfe_add_node(graph_clone, node_clone);
        
        node_clone->type = node->type;
        node_clone->number = node->number;
        node_clone->binary = node->binary;
        node_clone->assign = node->assign;

        if (node->string) {
            length = (strlen(node->string) + 1) * sizeof(char);

            node_clone->string = (char*)malloc(length);
            if (!node_clone->string) {
                sfe_delete(graph_clone);
                sfe_error_code = SFE_ALLOCATION_FAILED;
                return SFE_FALSE;
            }

            memcpy(node_clone->string, node->string, length);
        }

        if (node->unary) {
            node_clone->unary = (sfe_unary*)malloc(node->unary_count * sizeof(sfe_unary));
            if (!node_clone->unary) {
                sfe_delete(graph_clone);
                sfe_error_code = SFE_ALLOCATION_FAILED;
                return SFE_FALSE;
            }

            memcpy(node_clone->unary, node->unary, node->unary_count * sizeof(sfe_unary));
            node_clone->unary_count = node->unary_count;
        }

        if (increment) {
            if (node->pre) {
                node_clone->pre = (sfe_inc*)malloc(node->pre_count * sizeof(sfe_inc));
                if (!node_clone->pre) {
                    sfe_delete(graph_clone);
                    sfe_error_code = SFE_ALLOCATION_FAILED;
                    return SFE_FALSE;
                }

                memcpy(node_clone->pre, node->pre, node->pre_count * sizeof(sfe_inc));
                node_clone->pre_count = node->pre_count;
            }

            if (node->post) {
                node_clone->post = (sfe_inc*)malloc(node->post_count * sizeof(sfe_inc));
                if (!node_clone->post) {
                    sfe_delete(graph_clone);
                    sfe_error_code = SFE_ALLOCATION_FAILED;
                    return SFE_FALSE;
                }

                memcpy(node_clone->post, node->post, node->post_count * sizeof(sfe_inc));
                node_clone->post_count = node->post_count;
            }
        }

        if (node->subs) {
            node_clone->subs = (sfe_graph**)malloc(node->sub_count * sizeof(sfe_graph*));
            if (!node_clone->subs) {
                sfe_delete(graph_clone);
                sfe_error_code = SFE_ALLOCATION_FAILED;
                return SFE_FALSE;
            }
            node_clone->sub_count = node->sub_count;

            for (i = 0; i < node->sub_count; i++) {
                node_clone->subs[i] = sfe_do_clone(node->subs[i], node_clone, increment);
                if (!node_clone->subs[i]) {
                    sfe_delete(graph_clone);
                    return SFE_FALSE;
                }
            }
        }

        node = node->next;
    }

    return graph_clone;
}

sfe_graph* sfe_clone(sfe_graph* graph)
{
    return sfe_do_clone(graph, SFE_NULL, SFE_TRUE);
}

static sfe_num sfe_to_number(char* string)
{
    sfe_num number;
    char* end;
    sfe_int base;
    size_t i;
#ifdef SFEXPARSE_USE_FLOAT
    sfe_bool integer = SFE_TRUE;
#endif

    errno = 0;

    base = 10;
    if (string[0] == '0') {
        if (string[1]) {
            switch (string[1]) {
                case 'b':
                case 'B':
                    base = 2;
                    string += 2;
                    break;

                case 'o':
                case 'O':
                    base = 8;
                    string += 2;
                    break;

                case 'x':
                case 'X':
                    base = 16;
                    string += 2;
                    break;
            }
        }
    }

    switch (base) {
        case 2:
            for (i = 0; i < strlen(string); i++) {
                if (!(string[i] >= '0' && string[i] <= '1')) {
                    sfe_error_code = SFE_BAD_NUMBER;
                    return 0;
                }
            }
            break;

        case 8:
            for (i = 0; i < strlen(string); i++) {
                if (!(string[i] >= '0' && string[i] <= '7')) {
                    sfe_error_code = SFE_BAD_NUMBER;
                    return 0;
                }
            }
            break;

        case 10:
            for (i = 0; i < strlen(string); i++) {
#ifdef SFEXPARSE_USE_FLOAT
                if (string[i] == '.') {
                    integer = SFE_FALSE;
                    continue;
                }
#endif
                if (!(string[i] >= '0' && string[i] <= '9')) {
                    if (i == 0) {
                        sfe_error_code = SFE_NOT_NUMBER;
                    } else {
                        sfe_error_code = SFE_BAD_NUMBER;
                    }
                    return 0;
                }
            }
            break;

        case 16:
            for (i = 0; i < strlen(string); i++) {
                if (!(string[i] >= '0' && string[i] <= '9') &&
                    !(string[i] >= 'A' && string[i] <= 'F') && !(string[i] >= 'a' && string[i] <= 'f')) {
                    sfe_error_code = SFE_BAD_NUMBER;
                    return 0;
                }
            }
            break;
    }

#ifdef SFEXPARSE_USE_FLOAT
    if (!integer) {
        number = strtod(string, &end);
        if (errno != 0 || end == string) {
            sfe_error_code = SFE_NOT_NUMBER;
            return 0;
        }
        
        sfe_error_code = SFE_OK;
        return number;
    }
#endif

    number = strtol(string, &end, base);
    if (errno != 0 || end == string) {
        sfe_error_code = SFE_NOT_NUMBER;
        return 0;
    }

    sfe_error_code = SFE_OK;
    return number;
}

static char* sfe_sprintf(const char* format, sfe_num number)
{
    static char buffer[32];
    size_t length;
    char* string;

    length = ((size_t)sprintf(buffer, format, number) + 1) * sizeof(char);
    string = (char*)malloc(length);
    if (!string) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_NULL;
    }
    memcpy(string, buffer, length);

    sfe_error_code = SFE_OK;
    return string;
}

static char* sfe_from_number(sfe_num number)
{
    char* string;
#ifdef SFEXPARSE_USE_FLOAT
    size_t i;
#endif

#ifdef SFEXPARSE_USE_FLOAT
    string = sfe_sprintf("%f", number);
    if (!string) {
        return SFE_NULL;
    }

    for (i = strlen(string); i > 0; i--) {
        if (string[i - 1] == '.') {
            string[i - 1] = '\0';
            break;
        } else if (string[i - 1] != '0') {
            string[i] = '\0';
            break;
        }
    }
#else
    string = sfe_sprintf("%ld", number);
    if (!string) {
        return SFE_NULL;
    }
#endif

    sfe_error_code = SFE_OK;
    return string;
}

static char* sfe_append(char* base, const char* append)
{
    char* string;
    size_t length_1;
    size_t length_2;

    length_2 = strlen(append) + 1;
    if (!base) {
        length_1 = 0;
        string = (char*)malloc(length_2 * sizeof(char));
    } else {
        length_1 = strlen(base);
        string = (char*)realloc(base, (length_1 + length_2) * sizeof(char));
    }
    if (!string) {
        if (base) {
            free(base);
        }
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_NULL;
    }

    memcpy(string + length_1, append, length_2 * sizeof(char));

    sfe_error_code = SFE_OK;
    return string;
}

static sfe_bool sfe_get_value(sfe_node* node, sfe_value* value)
{
    sfe_error_code = SFE_CANT_GET_VALUE;

    switch (node->type) {
        case SFE_NUMBER:
            value->type = SFE_NUMBER;
            value->number = node->number;
            sfe_error_code = SFE_OK;
            break;

        case SFE_STRING:
            value->type = SFE_STRING;
            value->string = node->string;
            sfe_error_code = SFE_OK;
            break;

        default:
            if (sfe_do_get_value) {
                sfe_error_code = sfe_do_get_value(node, value);
            }
            break;
    }

    if (sfe_error_code != SFE_OK) {
        return SFE_FALSE;
    }
    return SFE_TRUE;
}

static sfe_bool sfe_assign(sfe_node* node, sfe_value* value)
{
    sfe_error_code = SFE_CANT_SET_VALUE;

    if (node->type != SFE_NUMBER && node->type != SFE_STRING) {
        if (sfe_do_set_value) {
            sfe_error_code = sfe_do_set_value(node, value);
        }
    }

    return (sfe_error_code == SFE_OK) ? SFE_TRUE : SFE_FALSE;
}

static void sfe_do_unary(sfe_node* node)
{
    sfe_uint i;

    if (node->type == SFE_NUMBER) {
        for (i = 0; i < node->unary_count; i++) {
            switch (node->unary[i]) {
                case SFE_POSITIVE:
                    node->number = +node->number;
                    break;

                case SFE_NEGATIVE:
                    if (node->number != 0) {
                        node->number = -node->number;
                    }
                    break;

                case SFE_LOGICAL_NOT:
                    node->number = (node->number == 0) ? 1 : 0;
                    break;

                case SFE_BITWISE_NOT:
                    node->number = (sfe_num)(~((sfe_int)node->number));
                    break;

                default:
                    break;
            }
        }
        
        if (node->unary) {
            free(node->unary);
            node->unary = SFE_NULL;
        }
        node->unary_count = 0;
    }
}

static sfe_bool sfe_do_increment(sfe_graph* graph, sfe_bool post)
{
    sfe_node* node;
    sfe_uint i;
    sfe_inc* increments;
    sfe_uint count;
    sfe_value value;

    memset(&value, 0, sizeof(sfe_value));

    node = graph->head;
    while (node) {
        if (node->subs) {
            for (i = 0; i < node->sub_count; i++) {
                if (!sfe_do_increment(node->subs[i], post)) {
                    return SFE_FALSE;
                }
            }
        }

        if (!post) {
            increments = node->pre;
            count = node->pre_count;
        } else {
            increments = node->post;
            count = node->post_count;
        }

        for (i = 0; i < count; i++) {
            if (!sfe_get_value(node, &value)) {
                return SFE_FALSE;
            }

            if (value.type == SFE_NUMBER) {
                if (increments[i] == SFE_INCREMENT) {
                    value.number++;
                } else {
                    value.number--;
                }
            } else {
                sfe_error_code = SFE_BAD_OPERATOR;
                return SFE_FALSE;
            }

            if (!sfe_assign(node, &value)) {
                return SFE_FALSE;
            }
        }

        node = node->next;
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_check_binary(sfe_node* left, sfe_node* right)
{
    if (left->type == SFE_STRING || right->type == SFE_STRING) {
        switch (left->binary) {
            case SFE_ADD:
                break;

            case SFE_EQUAL:
            case SFE_NOT_EQUAL:
            case SFE_LESS:
            case SFE_LESS_EQUAL:
            case SFE_GREATER:
            case SFE_GREATER_EQUAL:
                if (!left->assign && left->type != right->type) {
                    sfe_error_code = SFE_BAD_OPERATOR;
                    return SFE_FALSE;
                }
                break;

            default:
                sfe_error_code = SFE_BAD_OPERATOR;
                return SFE_FALSE;
        }
    } else if (right->type == SFE_NUMBER) {
         switch (left->binary) {
            case SFE_DIVIDE:
                if (right->number == 0) {
                    sfe_error_code = SFE_DIVIDE_BY_ZERO;
                    return SFE_FALSE;
                }
                break;

            case SFE_MODULO:
                if ((sfe_int)right->number == 0) {
                    sfe_error_code = SFE_DIVIDE_BY_ZERO;
                    return SFE_FALSE;
                }
                break;

            default:
                break;
        }
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_num sfe_do_binary(sfe_num left, sfe_num right, sfe_binary binary)
{
    sfe_uint i;
    sfe_num result;

    switch (binary) {
        case SFE_ADD:
            result = left + right;
            break;

        case SFE_SUBTRACT:
            result = left - right;
            break;

        case SFE_MULTIPLY:
            result = left * right;
            break;

        case SFE_EXPONENT:
            result = 1;
            if (right >= 0) {
                for (i = 0; i < (sfe_uint)right; i++) {
                    result *= left;
                }
            } else {
                for (i = 0; i < (sfe_uint)(-right); i++) {
                    result /= left;
                }
            }
            break;

        case SFE_DIVIDE:
            result = left / right;
            break;

        case SFE_MODULO:
            result = (sfe_num)((sfe_int)left % (sfe_int)right);
            break;

        case SFE_EQUAL:
            result = (left == right) ? 1 : 0;
            break;

        case SFE_NOT_EQUAL:
            result = (left != right) ? 1 : 0;
            break;

        case SFE_LESS:
            result = (left < right) ? 1 : 0;
            break;

        case SFE_LESS_EQUAL:
            result = (left <= right) ? 1 : 0;
            break;

        case SFE_GREATER:
            result = (left > right) ? 1 : 0;
            break;

        case SFE_GREATER_EQUAL:
            result = (left >= right) ? 1 : 0;
            break;

        case SFE_LOGICAL_AND:
            result = ((left != 0) && (right != 0)) ? 1 : 0;
            break;

        case SFE_LOGICAL_OR:
            result = ((left != 0) || (right != 0)) ? 1 : 0;
            break;

        case SFE_BITWISE_AND:
            result = (sfe_num)((sfe_int)left & (sfe_int)right);
            break;

        case SFE_BITWISE_OR:
            result = (sfe_num)((sfe_int)left | (sfe_int)right);
            break;

        case SFE_BITWISE_XOR:
            result = (sfe_num)((sfe_int)left ^ (sfe_int)right);
            break;

        case SFE_SHIFT_LEFT:
            result = (sfe_num)((sfe_int)left << (sfe_int)right);
            break;

        case SFE_SHIFT_RIGHT:
            result = (sfe_num)((sfe_int)left >> (sfe_int)right);
            break;

        default:
            result = 0;
            break;
    }

    return result;
}

static sfe_num sfe_do_compare(char* left, char* right, sfe_binary binary)
{
    sfe_num result;

    if (!left) {
        left = "";
    }
    if (!right) {
        right = "";
    }

    switch (binary) {
        case SFE_EQUAL:
            result = strcmp(left, right) ? 0 : 1;
            break;

        case SFE_NOT_EQUAL:
            result = strcmp(left, right) ? 1 : 0;
            break;

        case SFE_LESS:
            result = (strcmp(left, right) < 0) ? 1 : 0;
            break;

        case SFE_LESS_EQUAL:
            result = (strcmp(left, right) <= 0) ? 1 : 0;
            break;

        case SFE_GREATER:
            result = (strcmp(left, right) > 0) ? 1 : 0;
            break;

        case SFE_GREATER_EQUAL:
            result = (strcmp(left, right) >= 0) ? 1 : 0;
            break;

        default:
            result = 0;
            break;
    }

    return result;
}

static sfe_node* sfe_optimize_sub(sfe_node* node)
{
    sfe_node* head;
    sfe_node* tail;
    sfe_node* sub;
    sfe_bool optimize;
    sfe_unary* unary;

    if (node->type == SFE_SUBEXPRESSION) {
        head = node->subs[0]->head;
        tail = node->subs[0]->tail;

        optimize = SFE_FALSE;
        if (!node->pre && !node->post) {
            if (!node->previous && node->binary == SFE_BINARY_UNKNOWN) {
                if (!node->unary) {
                    optimize = SFE_TRUE;
                } else if (head->binary == SFE_BINARY_UNKNOWN) {
                    optimize = SFE_TRUE;
                }
            } else if (head->binary == SFE_BINARY_UNKNOWN) {
                optimize = SFE_TRUE;
            }
        }

        if (optimize) {
            if (head->binary == SFE_BINARY_UNKNOWN) {
                if (node->unary) {
                    if (head->unary) {
                        unary = (sfe_unary*)realloc(node->unary, ((size_t)node->unary_count + head->unary_count) * sizeof(sfe_unary));
                        if (!unary) {
                            sfe_error_code = SFE_ALLOCATION_FAILED;
                            return SFE_NULL;
                        }
                        node->unary = unary;

                        memcpy(node->unary + node->unary_count, head->unary, head->unary_count * sizeof(sfe_unary));
                        node->unary_count += head->unary_count;
                        free(head->unary);
                    }

                    head->unary = node->unary;
                    head->unary_count = node->unary_count;
                    node->unary = SFE_NULL;
                    node->unary_count = 0;
                }

                head->binary = node->binary;
                head->assign = node->assign;
            }

            sub = head;
            while (sub) {
                sub->graph = node->graph;
                sub = sub->next;
            }

            if (node->graph->head == node) {
                node->graph->head = head;
            }
            if (node->graph->tail == node) {
                node->graph->tail = tail;
            }
            if (node->previous) {
                node->previous->next = head;
            }
            if (node->next) {
                node->next->previous = tail;
            }
            head->previous = node->previous;
            tail->next = node->next;

            node->subs[0]->head = SFE_NULL;
            sfe_delete_node(node);

            node = head;
        }
    }

    sfe_error_code = SFE_OK;
    return node;
}

static sfe_bool sfe_add_group(sfe_node* node, sfe_group* groups, sfe_group_id id)
{
    sfe_node** group;

    group = (sfe_node**)realloc(groups[id].array, ((size_t)groups[id].count + 1) * sizeof(sfe_node*));
    if (!group) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_FALSE;
    }
    groups[id].array = group;
    groups[id].array[groups[id].count++] = node;

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_do_group_node(sfe_node* node, sfe_value* value)
{
    char* string;
    char* append;

    if (value->type == SFE_NUMBER) {
        if (node->next->type == SFE_NUMBER) {
            node->next->number = sfe_do_binary(value->number, node->next->number, node->binary);
        } else if (node->next->type == SFE_STRING) {
            string = sfe_from_number(value->number);
            if (!string) {
                return SFE_FALSE;
            }

            if (node->next->string) {
                string = sfe_append(string, node->next->string);
                if (!string) {
                    return SFE_FALSE;
                }
                free(node->next->string);
            }
            node->next->string = string;
        }
    } else if (value->type == SFE_STRING) {
        if (node->next->type == SFE_NUMBER) {
            string = sfe_clone_string(value->string);
            if (sfe_error_code != SFE_OK) {
                return SFE_FALSE;
            }
            append = sfe_from_number(node->next->number);
            if (!append) {
                free(string);
                return SFE_FALSE;
            }

            node->next->type = SFE_STRING;
            node->next->number = 0;
            node->next->string = sfe_append(string, append);
            free(append);
            if (!node->next->string) {
                return SFE_FALSE;
            }
        } else if (node->next->type == SFE_STRING) {
            if (node->binary == SFE_ADD) {
                string = sfe_clone_string(value->string);
                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }

                if (node->next->string) {
                    string = sfe_append(string, node->next->string);
                    if (!string) {
                        return SFE_FALSE;
                    }
                    free(node->next->string);
                }
                node->next->string = string;
            } else {
                node->next->type = SFE_NUMBER;
                node->next->number = sfe_do_compare(value->string, node->next->string, node->binary);
                if (node->next->string) {
                    free(node->next->string);
                    node->next->string = SFE_NULL;
                }
            }
        }
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_do_groups(sfe_group* groups, sfe_bool assign)
{
    sfe_uint i;
    sfe_uint j;
    sfe_node* node;
    sfe_value value;

    memset(&value, 0, sizeof(sfe_value));

    if (!assign) {
        for (i = 0; i < SFE_GROUP_COUNT; i++) {
            for (j = 0; j < groups[i].count; j++) {
                node = groups[i].array[j];

                value.type = node->type;
                value.number = node->number;
                value.string = node->string;

                if (!sfe_do_group_node(node, &value)) {
                    return SFE_FALSE;
                }
                sfe_delete_node(node);
            }
        }
    } else {
        for (i = 0; i < groups[0].count; i++) {
            node = groups[0].array[i];

            if (!sfe_get_value(node, &value)) {
                return SFE_FALSE;
            }
            
            if (node->binary != SFE_EQUAL) {
                if (value.type == SFE_NUMBER && node->next->type != SFE_NUMBER) {
                    sfe_error_code = SFE_BAD_OPERATOR;
                    return SFE_FALSE;
                } else if (!sfe_do_group_node(node, &value)) {
                    return SFE_FALSE;
                }
                value.number = node->next->number;
                value.string = node->next->string;
            } else {
                if (value.type == SFE_NUMBER) {
                    if (node->next->type == SFE_NUMBER) {
                        value.number = node->next->number;
                    } else {
                        sfe_error_code = SFE_BAD_OPERATOR;
                        return SFE_FALSE;
                    }
                } else {
                    if (node->next->type == SFE_NUMBER) {
                        node->next->type = SFE_STRING;
                        node->next->string = sfe_from_number(node->next->number);
                        if (!node->next->string) {
                            return SFE_FALSE;
                        }
                        node->next->number = 0;
                    }
                    value.string = node->next->string;
                }
            }

            if (!sfe_assign(node, &value)) {
                return SFE_FALSE;
            }
            sfe_delete_node(node);
        }
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_optimize(sfe_graph* graph);

static sfe_bool sfe_do_optimize(sfe_graph* graph, sfe_group* groups)
{
    sfe_node* node;
    sfe_uint i;

    node = graph->head;
    while (node) {
        for (i = 0; i < node->sub_count; i++) {
            if (!sfe_optimize(node->subs[i])) {
                return SFE_FALSE;
            } else if (i == 0) {
                node = sfe_optimize_sub(node);
                if (!node) {
                    return SFE_FALSE;
                }
                break;
            }
        }

        sfe_do_unary(node);
        if (node->previous) {
            if (!sfe_check_binary(node->previous, node)) {
                return SFE_FALSE;
            } else if ((node->type == SFE_NUMBER || node->type == SFE_STRING) &&
                (node->previous->type == SFE_NUMBER || node->previous->type == SFE_STRING)) {
                if (!sfe_add_group(node->previous, groups, sfe_group_ids[node->previous->binary])) {
                    return SFE_FALSE;
                }
            }
        }

        node = node->next;
    }

    return sfe_do_groups(groups, SFE_FALSE);
}

static sfe_bool sfe_optimize(sfe_graph* graph)
{
    sfe_group groups[SFE_GROUP_COUNT];
    sfe_uint i;
    sfe_bool success;

    memset(groups, 0, sizeof(groups));

    success = sfe_do_optimize(graph, groups);

    for (i = 0; i < SFE_GROUP_COUNT; i++) {
        if (groups[i].array) {
            free(groups[i].array);
        }
    }

    return success;
}

sfe_bool sfe_resolve_step(sfe_graph* graph, sfe_bool assign);

static sfe_bool sfe_do_resolve_step(sfe_graph* graph, sfe_bool assign, sfe_group* groups)
{
    sfe_node* node;
    sfe_value value;
    sfe_uint i;

    memset(&value, 0, sizeof(sfe_value));

    node = !assign ? graph->head : graph->tail;
    while (node) {
        for (i = 0; i < node->sub_count; i++) {
            if (!sfe_resolve_step(node->subs[i], assign)) {
                return SFE_FALSE;
            } else if (i == 0) {
                node = sfe_optimize_sub(node);
                if (!node) {
                    return SFE_FALSE;
                }
                break;
            }
        }

        if (node->type != SFE_NUMBER && node->type != SFE_STRING && !node->assign) {
            if (!sfe_get_value(node, &value)) {
                return SFE_FALSE;
            }

            node->type = value.type;
            node->number = value.number;
            if (node->string) {
                free(node->string);
            }
            node->string = value.string;
            sfe_delete_subs(node);
        }

        sfe_do_unary(node);
        if (node->previous) {
            if (!sfe_check_binary(node->previous, node)) {
                return SFE_FALSE;
            } else if (!assign && !node->previous->assign) {
                if (!sfe_add_group(node->previous, groups, sfe_group_ids[node->previous->binary])) {
                    return SFE_FALSE;
                }
            } else if (assign && node->previous->assign) {
                if (!sfe_add_group(node->previous, groups, SFE_GROUP_ASSIGN)) {
                    return SFE_FALSE;
                }
            }
        }

        node = !assign ? node->next : node->previous;
    }

    return sfe_do_groups(groups, assign);
}

sfe_bool sfe_resolve_step(sfe_graph* graph, sfe_bool assign)
{
    sfe_group groups[SFE_GROUP_COUNT];
    sfe_uint i;
    sfe_bool success;

    memset(groups, 0, sizeof(groups));

    success = sfe_do_resolve_step(graph, assign, groups);

    for (i = 0; i < SFE_GROUP_COUNT; i++) {
        if (groups[i].array) {
            free(groups[i].array);
        }
    }

    return success;
}

sfe_bool sfe_resolve(sfe_graph* graph, sfe_value* result)
{
    sfe_graph* resolve;

    if (!sfe_do_increment(graph, SFE_FALSE)) {
        return 0;
    }

    resolve = sfe_do_clone(graph, SFE_NULL, SFE_FALSE);
    if (!resolve) {
        return SFE_FALSE;
    }

    if (!sfe_resolve_step(resolve, SFE_FALSE)) {
        sfe_delete(resolve);
        return SFE_FALSE;
    } else if (!sfe_resolve_step(resolve, SFE_TRUE)) {
        sfe_delete(resolve);
        return SFE_FALSE;
    }

    if (!sfe_do_increment(graph, SFE_TRUE)) {
        sfe_delete(resolve);
        return SFE_FALSE;
    }

    if (resolve->head->type == SFE_NUMBER || resolve->head->type == SFE_STRING) {
        result->type = resolve->head->type;
        result->number = resolve->head->number;
        if (resolve->head->string) {
            result->string = sfe_clone_string(resolve->head->string);
        } else {
            result->string = SFE_NULL;
        }
    }

    sfe_delete(resolve);

    if (sfe_error_code != SFE_OK) {
        return SFE_FALSE;
    }
    return SFE_TRUE;
}

static sfe_bool sfe_add_unary(sfe_node* node, const sfe_unary unary)
{
    sfe_unary* array;

    if (node->pre) {
        sfe_error_code = SFE_BAD_OPERATOR;
        return SFE_FALSE;
    }
    
    array = (sfe_unary*)realloc(node->unary, ((size_t)node->unary_count + 1) * sizeof(sfe_unary));
    if (!array) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_FALSE;
    }
    node->unary = array;
    node->unary[node->unary_count++] = unary;

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_add_pre(sfe_node* node, sfe_inc pre)
{
    sfe_inc* array;

    array = (sfe_inc*)realloc(node->pre, ((size_t)node->pre_count + 1) * sizeof(sfe_inc));
    if (!array) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_FALSE;
    }
    node->pre = array;
    node->pre[node->pre_count++] = pre;

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_add_post(sfe_node* node, sfe_inc post)
{
    sfe_inc* array;

    if ((node->type != SFE_SYMBOL && node->type != SFE_ARRAY) || node->pre) {
        sfe_error_code = SFE_BAD_OPERATOR;
        return SFE_FALSE;
    }

    array = (sfe_inc*)realloc(node->post, ((size_t)node->post_count + 1) * sizeof(sfe_inc));
    if (!array) {
        sfe_error_code = SFE_ALLOCATION_FAILED;
        return SFE_FALSE;
    }
    node->post = array;
    node->post[node->post_count++] = post;

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_node* sfe_set_binary(sfe_node* node, sfe_binary binary, sfe_bool assign)
{
    sfe_node* next;

    if (node->type == SFE_TYPE_UNKNOWN) {
        if (node->unary || node->pre || node->post) {
            sfe_error_code = SFE_NO_OPERAND;
            return SFE_FALSE;
        } else {
            sfe_error_code = SFE_NO_LEFT_OPERAND;
            return SFE_NULL;
        }
    }

    node->binary = binary;
    node->assign = assign;

    node = sfe_optimize_sub(node);
    if (node == SFE_NULL) {
        return SFE_FALSE;
    }

    if (assign) {
        if (node->type != SFE_SYMBOL && node->type != SFE_ARRAY) {
            sfe_error_code = SFE_BAD_OPERATOR;
            return SFE_NULL;
        } else if (node->unary || node->pre || node->post) {
            sfe_error_code = SFE_BAD_OPERATOR;
            return SFE_NULL;
        }
    }
    
    next = sfe_create_node();
    if (!next) {
        return SFE_NULL;
    }
    sfe_add_node(node->graph, next);

    sfe_error_code = SFE_OK;
    return next;
}

static sfe_bool sfe_check_last(sfe_node* node)
{
    if (node->type == SFE_TYPE_UNKNOWN) {
        if (node->unary || node->pre || node->post) {
            sfe_error_code = SFE_NO_OPERAND;
            return SFE_FALSE;
        }
        
        if (node->previous) {
            sfe_error_code = SFE_NO_RIGHT_OPERAND;
            return SFE_FALSE;
        }
        
        if (node->graph->parent) {
            switch (node->graph->parent->type) {
                case SFE_SUBEXPRESSION:
                    sfe_error_code = SFE_NO_SUBEXPRESSION;
                    return SFE_FALSE;

                case SFE_FUNCTION:
                    if (node->graph->parent->sub_count > 1) {
                        sfe_error_code = SFE_NO_ARGUMENT;
                        return SFE_FALSE;
                    }
                    break;

                case SFE_ARRAY:
                    sfe_error_code = SFE_NO_SUBSCRIPT;
                    return SFE_FALSE;

                default:
                    sfe_error_code = SFE_BAD_NODE;
                    return SFE_FALSE;
            }
        }
    } else if (node->type == SFE_SUBEXPRESSION) {
        node = sfe_optimize_sub(node);
        if (node == SFE_NULL) {
            return SFE_FALSE;
        }
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_parse_string(sfe_work* work, sfe_node* node)
{
    size_t length;
    char* append;

    if (work->string_start) {
        length = work->string_end - work->string_start;

        append = malloc((length + 1) * sizeof(char));
        if (!append) {
            sfe_error_code = SFE_ALLOCATION_FAILED;
            return SFE_FALSE;
        }
        memcpy(append, work->string_start, length * sizeof(char));
        append[length] = '\0';

        node->string = sfe_append(node->string, append);
        free(append);
        if (!node->string) {
            return SFE_FALSE;
        }

        work->string_start = SFE_NULL;
        work->string_end = SFE_NULL;
    }

    node->type = SFE_STRING;
    return SFE_TRUE;
}

static sfe_bool sfe_parse_symbol(sfe_work* work, sfe_node* node)
{
    size_t length;
    sfe_num number;

    if (work->symbol_start) {
        length = work->symbol_end - work->symbol_start;
        node->string = (char*)malloc((length + 1) * sizeof(char));
        if (!node->string) {
            sfe_error_code = SFE_ALLOCATION_FAILED;
            return SFE_FALSE;
        }
        
        memcpy(node->string, work->symbol_start, length * sizeof(char));
        node->string[length] = '\0';

        number = sfe_to_number(node->string);

        switch (sfe_error_code) {
            case SFE_OK:
                node->type = SFE_NUMBER;
                node->number = number;
                free(node->string);
                node->string = SFE_NULL;

                if (node->pre) {
                    sfe_error_code = SFE_BAD_OPERATOR;
                    return SFE_FALSE;
                }
                break;

            case SFE_NOT_NUMBER:
                if (sfe_do_validate) {
                    if (!sfe_do_validate(node->string)) {
                        sfe_error_code = SFE_BAD_SYMBOL;
                        return SFE_FALSE;
                    }
                }
                node->type = SFE_SYMBOL;
                break;

            default:
                free(node->string);
                node->string = SFE_NULL;
                return SFE_FALSE;
        }

        work->symbol_start = SFE_NULL;
        work->symbol_end = SFE_NULL;
    }

    sfe_error_code = SFE_OK;
    return SFE_TRUE;
}

static sfe_bool sfe_do_parse(sfe_work* work, sfe_node* node)
{
    char string = '\0';
    char c;
    sfe_graph* sub;

    while ((c = *work->expression)) {
        work->expression++;

        if (string) {
            if (c == string) {
                if (!sfe_parse_string(work, node)) {
                    return SFE_FALSE;
                }
                string = '\0';
            } else {
                if (!work->string_start) {
                    work->string_start = work->expression - 1;
                }
                work->string_end = work->expression;
            }
            continue;
        }

        switch (c) {
            case '"':
            case '\'':
                if (node->unary || node->pre) {
                    sfe_error_code = SFE_BAD_OPERATOR;
                    return SFE_NULL;
                } else if (node->type != SFE_TYPE_UNKNOWN && node->type != SFE_STRING) {
                    sfe_error_code = SFE_NO_OPERATOR;
                    return SFE_FALSE;
                }
                string = c;
                break;

            case ' ':
            case '\t':
            case '\r':
            case '\n':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }
                break;

            case '(':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    node->type = SFE_SUBEXPRESSION;
                    sub = sfe_create_sub(node);
                    if (!sub) {
                        return SFE_FALSE;
                    } else if (!sfe_do_parse(work, sub->head)) {
                        return SFE_FALSE;
                    }
                    break;
                }

                if (node->type == SFE_SYMBOL) {
                    node->type = SFE_FUNCTION;
                    sub = sfe_create_sub(node);
                    if (!sub) {
                        return SFE_FALSE;
                    } else if (!sfe_do_parse(work, sub->head)) {
                        return SFE_FALSE;
                    } else if (node->sub_count == 1 && sfe_check_empty(sub)) {
                        sfe_delete_subs(node);
                    }
                    break;
                }

                sfe_error_code = SFE_NO_OPERATOR;
                return SFE_FALSE;

            case ')':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->graph->parent) {
                    if (node->graph->parent->type == SFE_SUBEXPRESSION || node->graph->parent->type == SFE_FUNCTION) {
                        return sfe_check_last(node);
                    }
                }

                sfe_error_code = SFE_NO_LEFT_PARENTHESIS;
                return SFE_FALSE;

            case '[':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    sfe_error_code = SFE_NO_ARRAY_NAME;
                    return SFE_FALSE;
                }

                if (node->type == SFE_SYMBOL || node->type == SFE_ARRAY) {
                    node->type = SFE_ARRAY;
                    sub = sfe_create_sub(node);
                    if (!sub) {
                        return SFE_FALSE;
                    } else if (!sfe_do_parse(work, sub->head)) {
                        return SFE_FALSE;
                    }
                    break;
                }

                sfe_error_code = SFE_BAD_SUBSCRIPT;
                return SFE_FALSE;

            case ']':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->graph->parent) {
                    if (node->graph->parent->type == SFE_ARRAY) {
                        return sfe_check_last(node);
                    }
                }

                sfe_error_code = SFE_NO_LEFT_BRACKET;
                return SFE_FALSE;

            case ',':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->graph->parent) {
                    if (node->graph->parent->type == SFE_FUNCTION) {
                        if (!sfe_check_last(node)) {
                            return SFE_FALSE;
                        } else {
                            sub = sfe_create_sub(node->graph->parent);
                            if (!sub) {
                                return SFE_FALSE;
                            }
                            node = sub->head;
                        }
                        break;
                    }
                }

                sfe_error_code = SFE_UNEXPECTED_COMMA;
                return SFE_FALSE;

            case '+':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    if (*work->expression == '+') {
                        work->expression++;
                        sfe_add_pre(node, SFE_INCREMENT);
                    } else {
                        sfe_add_unary(node, SFE_POSITIVE);
                    }
                } else {
                    switch (*work->expression) {
                        case '+':
                            work->expression++;
                            sfe_add_post(node, SFE_INCREMENT);
                            break;

                        case '=':
                            work->expression++;
                            node = sfe_set_binary(node, SFE_ADD, SFE_TRUE);
                            break;

                        default:
                            node = sfe_set_binary(node, SFE_ADD, SFE_FALSE);
                            break;
                    }
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '-':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    if (*work->expression == '-') {
                        work->expression++;
                        sfe_add_pre(node, SFE_DECREMENT);
                    } else {
                        sfe_add_unary(node, SFE_NEGATIVE);
                    }
                } else {
                    switch (*work->expression) {
                        case '-':
                            work->expression++;
                            sfe_add_post(node, SFE_DECREMENT);
                            break;

                        case '=':
                            work->expression++;
                            node = sfe_set_binary(node, SFE_SUBTRACT, SFE_TRUE);
                            break;

                        default:
                            node = sfe_set_binary(node, SFE_SUBTRACT, SFE_FALSE);
                            break;
                    }
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '*':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                switch (*work->expression) {
                    case '*':
                        if (*(++(work->expression)) == '=') {
                            work->expression++;
                            node = sfe_set_binary(node, SFE_EXPONENT, SFE_TRUE);
                        } else {
                            node = sfe_set_binary(node, SFE_EXPONENT, SFE_FALSE);
                        }
                        break;

                    case '=':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_MULTIPLY, SFE_TRUE);
                        break;

                    default:
                        node = sfe_set_binary(node, SFE_MULTIPLY, SFE_FALSE);
                        break;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '/':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (*work->expression == '=') {
                    work->expression++;
                    node = sfe_set_binary(node, SFE_DIVIDE, SFE_TRUE);
                } else {
                    node = sfe_set_binary(node, SFE_DIVIDE, SFE_FALSE);
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '%':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (*work->expression == '=') {
                    work->expression++;
                    node = sfe_set_binary(node, SFE_MODULO, SFE_TRUE);
                } else {
                    node = sfe_set_binary(node, SFE_MODULO, SFE_FALSE);
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '=':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (*work->expression == '=') {
                    work->expression++;
                    node = sfe_set_binary(node, SFE_EQUAL, SFE_FALSE);
                } else {
                    node = sfe_set_binary(node, SFE_EQUAL, SFE_TRUE);
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '!':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    sfe_add_unary(node, SFE_LOGICAL_NOT);
                } else if (*work->expression == '=') {
                    work->expression++;
                    node = sfe_set_binary(node, SFE_NOT_EQUAL, SFE_FALSE);
                } else {
                    sfe_error_code = SFE_BAD_OPERATOR;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '<':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                switch (*work->expression) {
                    case '<':
                        if (*(++(work->expression)) == '=') {
                            work->expression++;
                            node = sfe_set_binary(node, SFE_SHIFT_LEFT, SFE_TRUE);
                        } else {
                            node = sfe_set_binary(node, SFE_SHIFT_LEFT, SFE_FALSE);
                        }
                        break;

                    case '=':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_LESS_EQUAL, SFE_FALSE);
                        break;

                    default:
                        node = sfe_set_binary(node, SFE_LESS, SFE_FALSE);
                        break;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '>':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                switch (*work->expression) {
                    case '>':
                        if (*(++(work->expression)) == '=') {
                            work->expression++;
                            node = sfe_set_binary(node, SFE_SHIFT_RIGHT, SFE_TRUE);
                        } else {
                            node = sfe_set_binary(node, SFE_SHIFT_RIGHT, SFE_FALSE);
                        }
                        break;

                    case '=':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_GREATER_EQUAL, SFE_FALSE);
                        break;

                    default:
                        node = sfe_set_binary(node, SFE_GREATER, SFE_FALSE);
                        break;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '~':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (node->type == SFE_TYPE_UNKNOWN) {
                    sfe_add_unary(node, SFE_BITWISE_NOT);
                } else {
                    sfe_error_code = SFE_BAD_OPERATOR;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '&':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                switch (*work->expression) {
                    case '&':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_LOGICAL_AND, SFE_FALSE);
                        break;

                    case '=':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_BITWISE_AND, SFE_TRUE);
                        break;

                    default:
                        node = sfe_set_binary(node, SFE_BITWISE_AND, SFE_FALSE);
                        break;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '|':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                switch (*work->expression) {
                    case '|':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_LOGICAL_OR, SFE_FALSE);
                        break;

                    case '=':
                        work->expression++;
                        node = sfe_set_binary(node, SFE_BITWISE_OR, SFE_TRUE);
                        break;

                    default:
                        node = sfe_set_binary(node, SFE_BITWISE_OR, SFE_FALSE);
                        break;
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            case '^':
                if (!sfe_parse_symbol(work, node)) {
                    return SFE_FALSE;
                }

                if (*work->expression == '=') {
                    work->expression++;
                    node = sfe_set_binary(node, SFE_BITWISE_XOR, SFE_TRUE);
                } else {
                    node = sfe_set_binary(node, SFE_BITWISE_XOR, SFE_FALSE);
                }

                if (sfe_error_code != SFE_OK) {
                    return SFE_FALSE;
                }
                break;

            default:
                if (!work->symbol_start) {
                    if (node->type != SFE_TYPE_UNKNOWN) {
                        sfe_error_code = SFE_NO_OPERATOR;
                        return SFE_FALSE;
                    }
                    work->symbol_start = work->expression - 1;
                }
                work->symbol_end = work->expression;
                break;
        }
    }

    if (string) {
        sfe_error_code = SFE_NO_RIGHT_QUOTE;
        return SFE_FALSE;
    } else if (!sfe_parse_symbol(work, node)) {
        return SFE_FALSE;
    } else if (node->graph->parent) {
        switch (node->graph->parent->type) {
            case SFE_SUBEXPRESSION:
            case SFE_FUNCTION:
                sfe_error_code = SFE_NO_RIGHT_PARENTHESIS;
                return SFE_FALSE;

            case SFE_ARRAY:
                sfe_error_code = SFE_NO_RIGHT_BRACKET;
                return SFE_FALSE;

            default:
                sfe_error_code = SFE_BAD_NODE;
                return SFE_FALSE;
        }
    }
    return sfe_check_last(node);
}

sfe_graph* sfe_parse(const char* expression)
{
    sfe_work work;
    sfe_graph* graph;

    memset(&work, 0, sizeof(work));
    work.expression = expression;
    work.string_start = SFE_NULL;
    work.string_end = SFE_NULL;
    work.symbol_start = SFE_NULL;
    work.symbol_end = SFE_NULL;

    graph = sfe_create_graph(SFE_NULL, SFE_TRUE);
    if (!graph) {
        return SFE_NULL;
    }

    if (!sfe_do_parse(&work, graph->head)) {
        sfe_delete(graph);
        return SFE_NULL;
    } else if (sfe_check_empty(graph)) {
        sfe_delete(graph);
        return SFE_NULL;
    }

    if (!sfe_optimize(graph)) {
        sfe_delete(graph);
        return SFE_NULL;
    }

    return graph;
}

static char* sfe_do_to_string(sfe_graph* graph, char* string)
{
    sfe_node* node;
    sfe_uint i;
    char* number;
    sfe_bool space;
    sfe_unary check;

    static const char* unary[SFE_UNARY_COUNT] = {
        "+",
        "-",
        "!",
        "~"
    };

    static const char* increment[SFE_INCREMENT_COUNT] = {
        "++",
        "--"
    };

    static const char* binary[SFE_BINARY_COUNT][2] = {
        { " + ", " += " },
        { " - ", " -= " },
        { " * ", " *= " },
        { " ** ", " **= " },
        { " / ", " /= " },
        { " % ", " %= " },
        { " == ", " = " },
        { " != ", "" },
        { " < ", "" },
        { " <= ", "" },
        { " > ", "" },
        { " >= ", "" },
        { " && ", "" },
        { " || ", "" },
        { " & ", " &= " },
        { " | ", " |= " },
        { " ^ ", " ^= " },
        { " << ", " <<= " },
        { " >> ", " >>= " }
    };

    node = graph->head;
    while (node) {
        for (i = 0; i < node->unary_count; i++) {
            if (i > 0) {
                check = node->unary[i];
                if (node->unary[i - 1] == check && (check == SFE_POSITIVE || check == SFE_NEGATIVE)) {
                    string = sfe_append(string, " ");
                    if (!string) {
                        return SFE_NULL;
                    }
                }
            }

            string = sfe_append(string, unary[node->unary[i]]);
            if (!string) {
                return SFE_NULL;
            }
        }

        for (i = 0; i < node->pre_count; i++) {
            if (i == 0) {
                space = SFE_FALSE;
                if (node->unary) {
                    check = node->unary[node->unary_count - 1];
                    if (check == SFE_POSITIVE && node->pre[i] == SFE_INCREMENT) {
                        space = SFE_TRUE;
                    } else if (check == SFE_NEGATIVE && node->pre[i] == SFE_DECREMENT) {
                        space = SFE_TRUE;
                    }
                }

                if (space) {
                    string = sfe_append(string, " ");
                    if (!string) {
                        return SFE_NULL;
                    }
                }
            }

            string = sfe_append(string, increment[node->pre[i]]);
            if (!string) {
                return SFE_NULL;
            }
        }

        switch (node->type) {
            case SFE_NUMBER:
                number = sfe_from_number(node->number);
                if (!number) {
                    return SFE_NULL;
                }
                string = sfe_append(string, number);
                free(number);
                if (!string) {
                    return SFE_NULL;
                }
                break;

            case SFE_STRING:
                string = sfe_append(string, "\"");
                if (!string) {
                    return SFE_NULL;
                }
                if (node->string) {
                    string = sfe_append(string, node->string);
                    if (!string) {
                        return SFE_NULL;
                    }
                }
                string = sfe_append(string, "\"");
                if (!string) {
                    return SFE_NULL;
                }
                break;

            case SFE_SYMBOL:
                string = sfe_append(string, node->string);
                if (!string) {
                    return SFE_NULL;
                }
                break;

            case SFE_SUBEXPRESSION:
            case SFE_FUNCTION:
                string = sfe_append(string, node->string);
                if (!string) {
                    return SFE_NULL;
                }
                string = sfe_append(string, "(");
                if (!string) {
                    return SFE_NULL;
                }

            default:
                break;
        }
        
        for (i = 0; i < node->sub_count; i++) {
            if (node->type == SFE_ARRAY) {
                string = sfe_append(string, "[");
                if (!string) {
                    return SFE_NULL;
                }
            }

            string = sfe_do_to_string(node->subs[i], string);
            if (!string) {
                return SFE_NULL;
            }

            if (node->type == SFE_FUNCTION && i < (node->sub_count - 1)) {
                string = sfe_append(string, ", ");
                if (!string) {
                    return SFE_NULL;
                }
            } else if (node->type == SFE_ARRAY) {
                string = sfe_append(string, "]");
                if (!string) {
                    return SFE_NULL;
                }
            }
        }

        if (node->type == SFE_SUBEXPRESSION || node->type == SFE_FUNCTION) {
            string = sfe_append(string, ")");
            if (!string) {
                return SFE_NULL;
            }
        }

        for (i = 0; i < node->post_count; i++) {
            string = sfe_append(string, increment[node->post[i]]);
            if (!string) {
                return SFE_NULL;
            }
        }
        
        if (node->binary != SFE_BINARY_UNKNOWN) {
            string = sfe_append(string, binary[node->binary][(sfe_int)node->assign]);
            if (!string) {
                return SFE_NULL;
            }
        }

        node = node->next;
    }

    sfe_error_code = SFE_OK;
    return string;
}

char* sfe_to_string(sfe_graph* graph)
{
    return sfe_do_to_string(graph, SFE_NULL);
}

#endif
#ifdef __cplusplus
}
#endif
#endif
