/* queue functions (for forward/backward movement */

#include <glib.h>

#include "queue.h"

struct _Queue {
	GList  *queue;
	GList  *current;
};

Queue
queue_new(void)
{
	Queue  h;

	h = g_malloc(sizeof(*h));

	h->queue = NULL;
	h->current = NULL;
	return h;
}

static void
queue_free_element( char *data, gpointer foo )
{
	g_free(data);
}

void
queue_free(Queue h)
{
	g_return_if_fail( h != NULL );

	/* needs to free data in list as well! */
	if (h->queue) {
		g_list_foreach(h->queue, (GFunc)queue_free_element, NULL);
		g_list_free(h->queue);
	}

	g_free(h);
}

gchar
*queue_prev(Queue h)
{
	GList *p;

	if (!h || !h->queue || (h->current == g_list_first(h->queue)))
		return NULL;

	p = g_list_previous(h->current);

	
	h->current = p;

	return p->data;
}

gchar
*queue_next(Queue h)
{
	GList *p;


	if (!h || !h->queue || (h->current == g_list_last(h->queue)))
		return NULL;

	p = g_list_next(h->current);

	h->current = p;

	return p->data;
}

void 
queue_add(Queue h, gchar *ref)
{
	GList *trash=NULL;

	g_return_if_fail( h != NULL );
	g_return_if_fail( ref != NULL );

	if (h->current) {
		trash = h->current->next;
		h->current->next = NULL;
	}
		
	h->queue = g_list_append(h->queue, g_strdup(ref));
	h->current = g_list_last(h->queue);

	if (trash) {
		g_list_foreach(trash, (GFunc)queue_free_element, NULL);
		g_list_free(trash);
	}
	
}

gboolean
queue_isnext(Queue h)
{
	if (!h || !h->queue || (h->current == g_list_last(h->queue)))
		return FALSE;

	return (g_list_next(h->current) != NULL);
}

gboolean
queue_isprev(Queue h)
{
	if (!h || !h->queue || (h->current == g_list_first(h->queue)))
		return FALSE;

	return (g_list_previous(h->current) != NULL);
}


