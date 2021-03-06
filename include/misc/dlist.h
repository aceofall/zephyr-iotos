/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Doubly-linked list implementation
 *
 * Doubly-linked list implementation using inline macros/functions.
 * This API is not thread safe, and thus if a list is used across threads,
 * calls to functions must be protected with synchronization primitives.
 *
 * The lists are expected to be initialized such that both the head and tail
 * pointers point to the list itself.  Initializing the lists in such a fashion
 * simplifies the adding and removing of nodes to/from the list.
 */

#ifndef _misc_dlist__h_
#define _misc_dlist__h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// KID 20170517
// KID 20170527
// KID 20170602
// KID 20170717
// KID 20170719
// sizeof(struct _dnode): 8 bytes
struct _dnode {
	union {
		struct _dnode *head; /* ptr to head of list (sys_dlist_t) */
		struct _dnode *next; /* ptr to next node    (sys_dnode_t) */
	};
	union {
		struct _dnode *tail; /* ptr to tail of list (sys_dlist_t) */
		struct _dnode *prev; /* ptr to previous node (sys_dnode_t) */
	};
};

// KID 20170517
// KID 20170518
// KID 20170523
// KID 20170527
// KID 20170602
// KID 20170715
// KID 20170717
// KID 20170719
// KID 20170720
// sizeof(struct _dnode): 8 bytes
// sizeof(sys_dlist_t): 8 bytes
typedef struct _dnode sys_dlist_t;
// KID 20170517
// sizeof(struct _dnode): 8 bytes
// sizeof(sys_dnode_t): 8 bytes
typedef struct _dnode sys_dnode_t;

/**
 * @brief Provide the primitive to iterate on a list
 * Note: the loop is unsafe and thus __dn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_DLIST_FOR_EACH_NODE(l, n) {
 *         <user code>
 *     }
 *
 * This and other SYS_DLIST_*() macros are not thread safe.
 *
 * @param __dl A pointer on a sys_dlist_t to iterate on
 * @param __dn A sys_dnode_t pointer to peek each node of the list
 */
// KID 20170720
// wait_q_list: &(&(&k_sys_work_q)->fifo)->wait_q
// #define SYS_DLIST_FOR_EACH_NODE(&(&(&k_sys_work_q)->fifo)->wait_q, node):
// for (node = sys_dlist_peek_head(&(&(&k_sys_work_q)->fifo)->wait_q); node;
// 	node = sys_dlist_peek_next(&(&(&k_sys_work_q)->fifo)->wait_q, node))
#define SYS_DLIST_FOR_EACH_NODE(__dl, __dn)				\
	for (__dn = sys_dlist_peek_head(__dl); __dn;			\
	     __dn = sys_dlist_peek_next(__dl, __dn))

/**
 * @brief Provide the primitive to iterate on a list, from a node in the list
 * Note: the loop is unsafe and thus __dn should not be removed
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_DLIST_ITERATE_FROM_NODE(l, n) {
 *         <user code>
 *     }
 *
 * Like SYS_DLIST_FOR_EACH_NODE(), but __dn already contains a node in the list
 * where to start searching for the next entry from. If NULL, it starts from
 * the head.
 *
 * This and other SYS_DLIST_*() macros are not thread safe.
 *
 * @param __dl A pointer on a sys_dlist_t to iterate on
 * @param __dn A sys_dnode_t pointer to peek each node of the list;
 *             it contains the starting node, or NULL to start from the head
 */
#define SYS_DLIST_ITERATE_FROM_NODE(__dl, __dn) \
	for (__dn = __dn ? sys_dlist_peek_next_no_check(__dl, __dn) \
			 : sys_dlist_peek_head(__dl); \
	     __dn; \
	     __dn = sys_dlist_peek_next(__dl, __dn))

/**
 * @brief Provide the primitive to safely iterate on a list
 * Note: __dn can be removed, it will not break the loop.
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_DLIST_FOR_EACH_NODE_SAFE(l, n, s) {
 *         <user code>
 *     }
 *
 * This and other SYS_DLIST_*() macros are not thread safe.
 *
 * @param __dl A pointer on a sys_dlist_t to iterate on
 * @param __dn A sys_dnode_t pointer to peek each node of the list
 * @param __dns A sys_dnode_t pointer for the loop to run safely
 */
#define SYS_DLIST_FOR_EACH_NODE_SAFE(__dl, __dn, __dns)			\
	for (__dn = sys_dlist_peek_head(__dl),				\
		     __dns = sys_dlist_peek_next(__dl, __dn);  		\
	     __dn; __dn = __dns,					\
		     __dns = sys_dlist_peek_next(__dl, __dn))

/*
 * @brief Provide the primitive to resolve the container of a list node
 * Note: it is safe to use with NULL pointer nodes
 *
 * @param __dn A pointer on a sys_dnode_t to get its container
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_dnode_t within the container struct
 */
#define SYS_DLIST_CONTAINER(__dn, __cn, __n) \
	(__dn ? CONTAINER_OF(__dn, __typeof__(*__cn), __n) : NULL)
/*
 * @brief Provide the primitive to peek container of the list head
 *
 * @param __dl A pointer on a sys_dlist_t to peek
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_dnode_t within the container struct
 */
#define SYS_DLIST_PEEK_HEAD_CONTAINER(__dl, __cn, __n) \
	SYS_DLIST_CONTAINER(sys_dlist_peek_head(__dl), __cn, __n)

/*
 * @brief Provide the primitive to peek the next container
 *
 * @param __dl A pointer on a sys_dlist_t to peek
 * @param __cn Container struct type pointer
 * @param __n The field name of sys_dnode_t within the container struct
 */
#define SYS_DLIST_PEEK_NEXT_CONTAINER(__dl, __cn, __n) \
	((__cn) ? SYS_DLIST_CONTAINER(sys_dlist_peek_next(__dl, &(__cn->__n)), \
				      __cn, __n) : NULL)

/**
 * @brief Provide the primitive to iterate on a list under a container
 * Note: the loop is unsafe and thus __cn should not be detached
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_DLIST_FOR_EACH_CONTAINER(l, c, n) {
 *         <user code>
 *     }
 *
 * @param __dl A pointer on a sys_dlist_t to iterate on
 * @param __cn A pointer to peek each entry of the list
 * @param __n The field name of sys_dnode_t within the container struct
 */
#define SYS_DLIST_FOR_EACH_CONTAINER(__dl, __cn, __n)			\
	for (__cn = SYS_DLIST_PEEK_HEAD_CONTAINER(__dl, __cn, __n); __cn; \
	     __cn = SYS_DLIST_PEEK_NEXT_CONTAINER(__dl, __cn, __n))

/**
 * @brief Provide the primitive to safely iterate on a list under a container
 * Note: __cn can be detached, it will not break the loop.
 *
 * User _MUST_ add the loop statement curly braces enclosing its own code:
 *
 *     SYS_DLIST_FOR_EACH_CONTAINER_SAFE(l, c, cn, n) {
 *         <user code>
 *     }
 *
 * @param __dl A pointer on a sys_dlist_t to iterate on
 * @param __cn A pointer to peek each entry of the list
 * @param __cns A pointer for the loop to run safely
 * @param __n The field name of sys_dnode_t within the container struct
 */
#define SYS_DLIST_FOR_EACH_CONTAINER_SAFE(__dl, __cn, __cns, __n)	\
	for (__cn = SYS_DLIST_PEEK_HEAD_CONTAINER(__dl, __cn, __n),	\
	     __cns = SYS_DLIST_PEEK_NEXT_CONTAINER(__dl, __cn, __n); __cn; \
	     __cn = __cns,						\
	     __cns = SYS_DLIST_PEEK_NEXT_CONTAINER(__dl, __cn, __n))

/**
 * @brief initialize list
 *
 * @param list the doubly-linked list
 *
 * @return N/A
 */

// KID 20170519
// &_kernel.ready_q.q[0]
// KID 20170527
// &_timeout_q: &_kernel.timeout_q
// KID 20170717
// &queue->wait_q: &(&(&k_sys_work_q)->fifo)->wait_q
static inline void sys_dlist_init(sys_dlist_t *list)
{
	// list->head: (&_kernel.ready_q.q[0])->head, list: &_kernel.ready_q.q[0]
	// list->head: (&_kernel.timeout_q)->head, list: &_kernel.timeout_q
	list->head = (sys_dnode_t *)list;
	// list->head: (&_kernel.ready_q.q[0])->head: &_kernel.ready_q.q[0]
	// list->head: (&_kernel.timeout_q)->head: &_kernel.timeout_q

	// list->tail: (&_kernel.ready_q.q[0])->tail, list: &_kernel.ready_q.q[0]
	// list->tail: (&_kernel.timeout_q)->tail, list: &_kernel.timeout_q
	list->tail = (sys_dnode_t *)list;
	// list->tail: (&_kernel.ready_q.q[0])->tail: &_kernel.ready_q.q[0]
	// list->tail: (&_kernel.timeout_q)->tail: &_kernel.timeout_q
}

// KID 20170601
// SYS_DLIST_STATIC_INIT(&pipe_async_msgs.wait_q):
// {{(&pipe_async_msgs.wait_q)}, {(&pipe_async_msgs.wait_q)}}
#define SYS_DLIST_STATIC_INIT(ptr_to_list) {{(ptr_to_list)}, {(ptr_to_list)}}

/**
 * @brief check if a node is the list's head
 *
 * @param list the doubly-linked list to operate on
 * @param node the node to check
 *
 * @return 1 if node is the head, 0 otherwise
 */

static inline int sys_dlist_is_head(sys_dlist_t *list, sys_dnode_t *node)
{
	return list->head == node;
}

/**
 * @brief check if a node is the list's tail
 *
 * @param list the doubly-linked list to operate on
 * @param node the node to check
 *
 * @return 1 if node is the tail, 0 otherwise
 */

static inline int sys_dlist_is_tail(sys_dlist_t *list, sys_dnode_t *node)
{
	return list->tail == node;
}

/**
 * @brief check if the list is empty
 *
 * @param list the doubly-linked list to operate on
 *
 * @return 1 if empty, 0 otherwise
 */

// KID 20170602
// list: &(&pipe_async_msgs)->wait_q
// KID 20170720
// q: &_kernel.ready_q.q[15]
// KID 20170720
// list: &_ready_q.q[16]: &_kernel.ready_q.q[16]
// ARM10C 20170720
// &(&(&k_sys_work_q)->fifo)->wait_q
static inline int sys_dlist_is_empty(sys_dlist_t *list)
{
	// list->head: (&(&pipe_async_msgs)->wait_q)->head: &pipe_async_msgs.wait_q,
	// list: &(&pipe_async_msgs)->wait_q
	// list->head: (&_kernel.ready_q.q[15])->head: &_kernel.ready_q.q[15],
	// list: &_kernel.ready_q.q[15]
	// list->head: (&_kernel.ready_q.q[16])->head: &(&_main_thread_s)->base.k_q_node
	// list: &_kernel.ready_q.q[16]
	// list->head: (&(&(&k_sys_work_q)->fifo)->wait_q)->head: &(&(&k_sys_work_q)->fifo)->wait_q
	// list: &(&(&k_sys_work_q)->fifo)->wait_q
	return list->head == list;
	// return 1
	// return 1
	// return 0
	// return 1
}

/**
 * @brief check if more than one node present
 *
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 *
 * @return 1 if multiple nodes, 0 otherwise
 */

static inline int sys_dlist_has_multiple_nodes(sys_dlist_t *list)
{
	return list->head != list->tail;
}

/**
 * @brief get a reference to the head item in the list
 *
 * @param list the doubly-linked list to operate on
 *
 * @return a pointer to the head element, NULL if list is empty
 */

// KID 20170602
// wait_q: &(&pipe_async_msgs)->wait_q
// KID 20170720
// &(&(&k_sys_work_q)->fifo)->wait_q
static inline sys_dnode_t *sys_dlist_peek_head(sys_dlist_t *list)
{
	// list: &(&pipe_async_msgs)->wait_q
	// sys_dlist_is_empty(&(&pipe_async_msgs)->wait_q): 1
	// list: &(&(&k_sys_work_q)->fifo)->wait_q
	// sys_dlist_is_empty(&(&(&k_sys_work_q)->fifo)->wait_q): 1
	return sys_dlist_is_empty(list) ? NULL : list->head;
	// return NULL
	// return NULL
}

/**
 * @brief get a reference to the head item in the list
 *
 * The list must be known to be non-empty.
 *
 * @param list the doubly-linked list to operate on
 *
 * @return a pointer to the head element
 */

// KID 20170720
// list: &_ready_q.q[16]: &_kernel.ready_q.q[16]
static inline sys_dnode_t *sys_dlist_peek_head_not_empty(sys_dlist_t *list)
{
	// list->head: (&_kernel.ready_q.q[16])->head: &(&_main_thread_s)->base.k_q_node
	return list->head;
	// return &(&_main_thread_s)->base.k_q_node
}

/**
 * @brief get a reference to the next item in the list, node is not NULL
 *
 * Faster than sys_dlist_peek_next() if node is known not to be NULL.
 *
 * @param list the doubly-linked list to operate on
 * @param node the node from which to get the next element in the list
 *
 * @return a pointer to the next element from a node, NULL if node is the tail
 */

static inline sys_dnode_t *sys_dlist_peek_next_no_check(sys_dlist_t *list,
							sys_dnode_t *node)
{
	return (node == list->tail) ? NULL : node->next;
}

/**
 * @brief get a reference to the next item in the list
 *
 * @param list the doubly-linked list to operate on
 * @param node the node from which to get the next element in the list
 *
 * @return a pointer to the next element from a node, NULL if node is the tail
 * or NULL (when node comes from reading the head of an empty list).
 */

static inline sys_dnode_t *sys_dlist_peek_next(sys_dlist_t *list,
					       sys_dnode_t *node)
{
	return node ? sys_dlist_peek_next_no_check(list, node) : NULL;
}

/**
 * @brief get a reference to the tail item in the list
 *
 * @param list the doubly-linked list to operate on
 *
 * @return a pointer to the tail element, NULL if list is empty
 */

static inline sys_dnode_t *sys_dlist_peek_tail(sys_dlist_t *list)
{
	return sys_dlist_is_empty(list) ? NULL : list->tail;
}

/**
 * @brief add node to tail of list
 *
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to append
 *
 * @return N/A
 */

// KID 20170523
// q: &_kernel.ready_q.q[16], &thread->base.k_q_node: &(&_main_thread_s)->base.k_q_node
// KID 20170526
// q: &_kernel.ready_q.q[31], &thread->base.k_q_node: &(&_idle_thread_s)->base.k_q_node
// KID 20170717
// q: &_kernel.ready_q.q[15], &thread->base.k_q_node: &(&(&k_sys_work_q)->thread)->base.k_q_node
// KID 20170720
// wait_q_list: &(&(&k_sys_work_q)->fifo)->wait_q,
// &thread->base.k_q_node: &(&(&k_sys_work_q)->thread)->base.k_q_node
static inline void sys_dlist_append(sys_dlist_t *list, sys_dnode_t *node)
{
	// node->next: (&(&_main_thread_s)->base.k_q_node)->next, list: &_kernel.ready_q.q[16]
	// node->next: (&(&_idle_thread_s)->base.k_q_node)->next, list: &_kernel.ready_q.q[31]
	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next, list: &_kernel.ready_q.q[15]
	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next, list: &(&(&k_sys_work_q)->fifo)->wait_q
	node->next = list;
	// node->next: (&(&_main_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[16]
	// node->next: (&(&_idle_thread_s)->base.k_q_node)->next: &_kernel.ready_q.q[31]
	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next: &_kernel.ready_q.q[15]
	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next, &(&(&k_sys_work_q)->fifo)->wait_q

	// node->prev: (&(&_main_thread_s)->base.k_q_node)->prev, list->tail: (&_kernel.ready_q.q[16])->tail
	// node->prev: (&(&_idle_thread_s)->base.k_q_node)->prev, list->tail: (&_kernel.ready_q.q[31])->tail
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev, list->tail: (&_kernel.ready_q.q[15])->tail
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev, list->tail: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail
	node->prev = list->tail;
	// node->prev: (&(&_main_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[16])->tail
	// node->prev: (&(&_idle_thread_s)->base.k_q_node)->prev: (&_kernel.ready_q.q[31])->tail
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&_kernel.ready_q.q[15])->tail
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail

	// list->tail->next: (&_kernel.ready_q.q[16])->tail->next, node: &(&_main_thread_s)->base.k_q_node
	// list->tail->next: (&_kernel.ready_q.q[31])->tail->next, node: &(&_idle_thread_s)->base.k_q_node
	// list->tail->next: (&_kernel.ready_q.q[15])->tail->next, node: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// list->tail->next: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail->next, node: &(&(&k_sys_work_q)->thread)->base.k_q_node
	list->tail->next = node;
	// list->tail: (&_kernel.ready_q.q[16])->tail->next: &(&_main_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[31])->tail->next: &(&_idle_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[15])->tail->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// list->tail->next: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail->next: &(&(&k_sys_work_q)->thread)->base.k_q_node

	// list->tail: (&_kernel.ready_q.q[16])->tail, node: &(&_main_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[31])->tail, node: &(&_idle_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[15])->tail, node: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// list->tail: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail, node: &(&(&k_sys_work_q)->thread)->base.k_q_node
	list->tail = node;
	// list->tail: (&_kernel.ready_q.q[16])->tail: &(&_main_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[31])->tail: &(&_idle_thread_s)->base.k_q_node
	// list->tail: (&_kernel.ready_q.q[15])->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// list->tail: (&(&(&k_sys_work_q)->fifo)->wait_q)->tail: &(&(&k_sys_work_q)->thread)->base.k_q_node
}

/**
 * @brief add node to head of list
 *
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to append
 *
 * @return N/A
 */

static inline void sys_dlist_prepend(sys_dlist_t *list, sys_dnode_t *node)
{
	node->next = list->head;
	node->prev = list;

	list->head->prev = node;
	list->head = node;
}

/**
 * @brief insert node after a node
 *
 * Insert a node after a specified node in a list.
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 * @param insert_point the insert point in the list: if NULL, insert at head
 * @param node the element to append
 *
 * @return N/A
 */

static inline void sys_dlist_insert_after(sys_dlist_t *list,
	sys_dnode_t *insert_point, sys_dnode_t *node)
{
	if (!insert_point) {
		sys_dlist_prepend(list, node);
	} else {
		node->next = insert_point->next;
		node->prev = insert_point;
		insert_point->next->prev = node;
		insert_point->next = node;
	}
}

/**
 * @brief insert node before a node
 *
 * Insert a node before a specified node in a list.
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 * @param insert_point the insert point in the list: if NULL, insert at tail
 * @param node the element to insert
 *
 * @return N/A
 */

static inline void sys_dlist_insert_before(sys_dlist_t *list,
	sys_dnode_t *insert_point, sys_dnode_t *node)
{
	if (!insert_point) {
		sys_dlist_append(list, node);
	} else {
		node->prev = insert_point->prev;
		node->next = insert_point;
		insert_point->prev->next = node;
		insert_point->prev = node;
	}
}

/**
 * @brief insert node at position
 *
 * Insert a node in a location depending on a external condition. The cond()
 * function checks if the node is to be inserted _before_ the current node
 * against which it is checked.
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 * @param node the element to insert
 * @param cond a function that determines if the current node is the correct
 *             insert point
 * @param data parameter to cond()
 *
 * @return N/A
 */

static inline void sys_dlist_insert_at(sys_dlist_t *list, sys_dnode_t *node,
	int (*cond)(sys_dnode_t *, void *), void *data)
{
	if (sys_dlist_is_empty(list)) {
		sys_dlist_append(list, node);
	} else {
		sys_dnode_t *pos = sys_dlist_peek_head(list);

		while (pos && !cond(pos, data)) {
			pos = sys_dlist_peek_next(list, pos);
		}
		sys_dlist_insert_before(list, pos, node);
	}
}

/**
 * @brief remove a specific node from a list
 *
 * The list is implicit from the node. The node must be part of a list.
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param node the node to remove
 *
 * @return N/A
 */

// KID 20170719
// &thread->base.k_q_node: &(&(&k_sys_work_q)->thread)->base.k_q_node
static inline void sys_dlist_remove(sys_dnode_t *node)
{
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: &_kernel.ready_q.q[15]
	// node->prev->next: (&_kernel.ready_q.q[15])->next: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next: &_kernel.ready_q.q[15]
	node->prev->next = node->next;
	// node->prev->next: (&_kernel.ready_q.q[15])->next: &_kernel.ready_q.q[15]

	// node->next: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->next: &_kernel.ready_q.q[15]
	// node->next->prev: (&_kernel.ready_q.q[15])->prev: &(&(&k_sys_work_q)->thread)->base.k_q_node
	// node->prev: (&(&(&k_sys_work_q)->thread)->base.k_q_node)->prev: &_kernel.ready_q.q[15]
	node->next->prev = node->prev;
	// node->next->prev: (&_kernel.ready_q.q[15])->prev: &_kernel.ready_q.q[15]
}

/**
 * @brief get the first node in a list
 *
 * This and other sys_dlist_*() functions are not thread safe.
 *
 * @param list the doubly-linked list to operate on
 *
 * @return the first node in the list, NULL if list is empty
 */

static inline sys_dnode_t *sys_dlist_get(sys_dlist_t *list)
{
	sys_dnode_t *node;

	if (sys_dlist_is_empty(list)) {
		return NULL;
	}

	node = list->head;
	sys_dlist_remove(node);
	return node;
}

#ifdef __cplusplus
}
#endif

#endif /* _misc_dlist__h_ */
