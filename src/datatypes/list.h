#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

/**
 * @defgroup list list.h
 * @brief The header file for most the functions used in the list.
 * The list construction is double ended and stores one value for the user
 * to store information in. It also contains two pointers to move forward and
 * backwards. Dynamic memory is used in these following functions so the user
 * has to be caustios with memory usage.
 *
 * @author Mert Karkan dv19TKN
 * @{
 */

/**
 * @brief The structure for a "node".
 *
 * The structer has a pointer to node called "next", which is pointing
 * to the next node in the list.
 *
 * There is a pointer to node called "prev", which is pointing to
 * the previous node in the list.
 *
 * There is also "value" which is a pointer to char called "value",
 * that stores the users value.
 *
 */
struct node
{
    struct node *next;
    struct node *prev;
    char *ssn;
    char *email;
    char *name;
};

/**
 * @brief The structure for a "list".
 *
 * The structure has a "head" of type "node", which refers to the first
 * element in the list.
 *
 * <b>OBS</b>: The head can also point to "prev" which refers to the last element
 * in the list.
 */
typedef struct list
{
    struct node head;
} List;

/**
 * @brief The struct for a "list_pos".
 *
 * The structer has a pointer to "node", which refers to the position in
 * the list.
 *
 */
typedef struct list_pos
{
    struct node *node;
} ListPos;

/**
 * @brief Creates a list.
 *
 * Creates the head of the list. The head is refered to as the first elements
 * in the list, which is needed to keep track on the elemnts position in the
 * list.
 *
 * <b>OBS</b>: The user has to free up memory either with a function or manually.
 * @param Void
 * @return *List A pointer to the head of the list
 */
List *list_create(void);

/**
 * @brief Deallocate the list.
 *
 * Deallocate the list (and all of its values, if any) using the function
 * "list_remove". This function also uses "list_first" and "list_next" to go
 * through each element in the list.
 *
 * <b>OBS</b>: Freeing up memory, use cautiously.
 * @param List* Pointer to a list.
 * @return Void
 */
void list_destroy(List *lst);

/**
 * @brief Checks if the list is empty.
 *
 * Checks if the list is empty by looking at next element of the head.
 * Returns "true" if list is empty, or "false" otherwise.
 * @param List* A pointer to the head of the list.
 * @return Bool True if the list is empty.
 */
bool list_is_empty(const List *lst);

/**
 * @brief Gets the position of the first element.
 *
 * Gets the position of the first element for a given list, which is the
 * element next to head.
 *
 * @param List* A pointer to the head of the list.
 * @return ListPos The position of the first elemennt in the list.
 */
ListPos list_first(List *lst);

/**
 * @brief Get the position <b>after</b> the last element.
 *
 * Get the position <b>after</b> the last element, which is also the head
 * of the list.
 *
 * @param List A pointer to the head of the list.
 * @return ListPos The position of the head of the list.
 */
ListPos list_end(List *lst);

/**
 * @brief Check equality between two positions.
 *
 * Check equality between two given positions in the list. Returns "true"
 * as a boolean value if the nodes are equal.
 *
 * @param ListPos The first position.
 * @param ListPos The second position.
 * @return Bool Returns true if the positions are equal, else false.
 */
bool list_pos_equal(ListPos p1, ListPos p2);

/**
 * @brief Goes to the next position.
 *
 * Goes to the next position in the list for a given node.
 *
 * @param ListPos The current position.
 * @return ListPos The position next to the current position.
 */
ListPos list_next(ListPos pos);

/**
 * @brief Goes to the previous position.
 *
 * Goes to the previous position in the list for a given node.
 *
 * @param ListPos The current position.
 * @return ListPos The previous position to the current position.
 */
ListPos list_prev(ListPos pos);

/**
 * @brief Inserts the given value and returns the position of the new element.
 *
 * Insert the value before the position and return the position of the new
 * element.
 *
 * @param ListPos The position of the element.
 * @param Char* The value to be added.
 * @return ListPos The position of the new element.
 */
ListPos list_insert(ListPos pos, const char *ssn, const char *email, const char *name);

/**
 * @brief Removes the node for the given position.
 *
 * Removes the value at the position and return the position of the next
 * element in the list.
 *
 * <b>OBS</b>: Freeing up memory, use cautiously.
 *
 * @param ListPos The position of the element.
 * @return ListPos The position next to the removed element.
 */
ListPos list_remove(ListPos pos);

/**
 * @brief Gets the social security number at that position.
 *
 * Gets the social security number of the person at that 
 * position of the list.
 *
 * @param ListPos The position of the element.
 * @return Char The SSN of the person.
 */
const char *list_inspect_ssn(ListPos pos);

/**
 * @brief Gets the E-Mail at that position.
 *
 * Gets the E-Mail of the person at that 
 * position of the list.
 *
 * @param ListPos The position of the element.
 * @return Char The email of the person.
 */
const char *list_inspect_email(ListPos pos);

/**
 * @brief Gets the name at the position.
 *
 * Gets the name at the given position in the list.
 *
 * @param ListPos The position of the element.
 * @return Char The name at the given position.
 */
const char *list_inspect_name(ListPos pos);

/**
 * Returns the length of the list.
 * 
 * @param List
 * @return int Length of the list.  
 */
int list_get_length(List *lst);

/**
 * @}
 */

#endif /* LIST_H */
