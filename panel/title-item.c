/* Generated by GOB (v1.0.10) on Thu Sep 20 22:11:34 2001
   (do not edit directly) */

/* End world hunger, donate to the World Food Programme, http://www.wfp.org */

#define GOB_VERSION_MAJOR 1
#define GOB_VERSION_MINOR 0
#define GOB_VERSION_PATCHLEVEL 10

#include "title-item.h"

/* self casting macros */
#define SELF(x) TITLE_ITEM(x)
#define SELF_CONST(x) TITLE_ITEM_CONST(x)
#define IS_SELF(x) IS_TITLE_ITEM(x)
#define TYPE_SELF TYPE_TITLE_ITEM
#define SELF_CLASS(x) TITLE_ITEM_CLASS(x)

#define SELF_GET_CLASS(x) TITLE_ITEM_GET_CLASS(x)

/* self typedefs */
typedef TitleItem Self;
typedef TitleItemClass SelfClass;

/* GTK_CLASS_TYPE for 1.2<->1.3/2.0 GTK+ compatibility */
#ifndef GTK_CLASS_TYPE
#define GTK_CLASS_TYPE(x) (GTK_OBJECT_CLASS(x)->type)
#endif /* GTK_CLASS_TYPE */

/* here are local prototypes */
static void title_item_init (TitleItem * o) G_GNUC_UNUSED;
static void title_item_class_init (TitleItemClass * c) G_GNUC_UNUSED;

/* pointer to the class of our parent */
static GtkMenuItemClass *parent_class = NULL;

GtkType
title_item_get_type (void)
{
	static GtkType type = 0;

	if (type == 0) {
		static const GtkTypeInfo info = {
			"TitleItem",
			sizeof (TitleItem),
			sizeof (TitleItemClass),
			(GtkClassInitFunc) title_item_class_init,
			(GtkObjectInitFunc) title_item_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_menu_item_get_type(), &info);
	}

	return type;
}

/* Short form macros */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define new() title_item_new()
#endif /* __GNUC__ && !__STRICT_ANSI__ */

/* Short form pointers */
static GtkWidget * (* const new) (void) = title_item_new;

/* a macro for creating a new object of our type */
#define GET_NEW ((TitleItem *)gtk_type_new(title_item_get_type()))

static void 
title_item_init (TitleItem * o)
{
#define __GOB_FUNCTION__ ":Title:Item::init"
	return;
	o = NULL;
}
#undef __GOB_FUNCTION__
static void 
title_item_class_init (TitleItemClass * c)
{
#define __GOB_FUNCTION__ ":Title:Item::class_init"
	GtkItemClass *gtk_item_class = (GtkItemClass *)c;

	parent_class = gtk_type_class (gtk_menu_item_get_type ());

	gtk_item_class->select = NULL;
	return;
	c = NULL;
}
#undef __GOB_FUNCTION__




#line 5 "title-item.gob"
GtkWidget * 
title_item_new (void)
#line 100 "title-item.c"
{
#define __GOB_FUNCTION__ ":Title:Item::new"
{
#line 7 "title-item.gob"
	
		return GTK_WIDGET(GET_NEW);
	}}
#line 108 "title-item.c"
#undef __GOB_FUNCTION__


#if (!defined __GNUC__) || (defined __GNUC__ && defined __STRICT_ANSI__)
/*REALLY BAD HACK
  This is to avoid unused warnings if you don't call
  some method.  I need to find a better way to do
  this, not needed in GCC since we use some gcc
  extentions to make saner, faster code */
static void
___title_item_really_bad_hack_to_avoid_warnings(void)
{
	((void (*)(void))new)();
	___title_item_really_bad_hack_to_avoid_warnings();
}
#endif /* !__GNUC__ || (__GNUC__ && __STRICT_ANSI__) */

