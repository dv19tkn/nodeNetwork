#include <stdlib.h>
#include <string.h>
#include "list.h"

/**
 * @defgroup list_static Static_List
 *
 * @brief "list.c" contains the functions used to make a double ended list.
 * The following lists are using dynamic memory, use it with caustion.
 *
 * @author Mert Karkan dv19TKN
 * @{
 */

/**
  *
  * @brief Clones the given string.
  *
  * Gets the value at the given position in the list and returns a copy of it.
  *
  * @param Char* The string to be copied.
  * @return Char* A copy of the given string.
  */
static char *clone_string(const char *in)
{
    size_t len = strlen(in);
    char *out = calloc(len + 1, sizeof(char));
    strcpy(out, in);
    return out;
}

/**
 * @brief Makes a node.
 *
 * Creates a node and puts in the value given. The pointers "next" and "prev"
 * points to NULL on creation.
 *
 * <b>OBS</b>: The user has to free up memory either with a function or
 * manually when destroying the node.
 *
 * @param Char* The value of the node.
 * @return Char* A node where the pointers "next" and "prev" points to NULL.
 */
static struct node *make_node(const char *ssn, const char *email, const char *name)
{
    struct node *new_node = malloc(sizeof(struct node));
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->ssn = clone_string(ssn);
    new_node->email = clone_string(email);
    new_node->name = clone_string(name);

    return new_node;
}

/**
 * @}
 */

//(The user has to free up memory.)
List *list_create(void)
{
    List *lst = malloc(sizeof(List));
    lst->head = (struct node){
        .next = &lst->head,
        .prev = &lst->head,
        .ssn = NULL,
        .email = NULL,
        .name = NULL};

    return lst;
}

//(FREEING UP MEMORY.)
void list_destroy(List *lst)
{
    ListPos pos = list_first(lst);

    while (!list_is_empty(lst))
    {
        pos = list_remove(pos);
    }

    free(lst);
}

bool list_is_empty(const List *lst)
{
    return lst->head.next == &lst->head;
}

ListPos list_first(List *lst)
{
    ListPos pos = {
        .node = lst->head.next};

    return pos;
}

ListPos list_end(List *lst)
{
    ListPos pos = {
        .node = &lst->head};

    return pos;
}

bool list_pos_equal(ListPos p1, ListPos p2)
{
    if (p1.node == p2.node)
    {
        return true;
    }
    return false;
}

ListPos list_next(ListPos pos)
{
    pos.node = pos.node->next;
    return pos;
}

ListPos list_prev(ListPos pos)
{
    pos.node = pos.node->prev;
    return pos;
}

ListPos list_insert(ListPos pos, const char *ssn, const char *email, const char *name)
{
    // Create a new node.
    struct node *node = make_node(ssn, email, name);

    // Find nodes before and after (may be the same node: the head of the list).
    struct node *before = pos.node->prev;
    struct node *after = pos.node;

    // Link to node after.
    node->next = after;
    after->prev = node;

    // Link to node before.
    node->prev = before;
    before->next = node;

    // Return the position of the new element.
    pos.node = node;
    return pos;
}

//(FREEING UP MEMORY.)
ListPos list_remove(ListPos pos)
{
    pos.node->prev->next = pos.node->next;
    pos.node->next->prev = pos.node->prev;

    ListPos new_pos = {pos.node->next};

    //testa och se ifall det funkar utan
    free(pos.node->ssn);
    free(pos.node->email);
    free(pos.node->name);
    free(pos.node);

    return new_pos;
}

const char *list_inspect_ssn(ListPos pos)
{
    char *ssn = pos.node->ssn;
    return ssn;
}

const char *list_inspect_email(ListPos pos)
{
    char *email = pos.node->email;
    return email;
}

const char *list_inspect_name(ListPos pos)
{
    char *name = pos.node->name;
    return name;
}

int list_get_length(List *lst)
{
    ListPos pos = list_first(lst);
    int len = 0;

    if (!list_is_empty(lst))
    {
        while (!list_pos_equal(pos, list_end(lst)))
        {
            len++;
            pos = list_next(pos);
        }
    }

    return len;
}
