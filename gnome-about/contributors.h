#include <gnome.h>

/* If your name uses some letters not in 7bit ascii, or a non-lati script;
 * put your name in ascii enclosed with N_( ) and put a comment before your
 * line giving some info on how it should be written for languages that
 * support the proper letters/script for your name. So for example someone
 * with a name normally written in cyrillic can have it displayed in cyrillic
 * for those languages supporting it */ 


/* Please keep this in alphabetical order */
static gchar *contributors[] = {
	N_("GNOME was brought to you by"),
	" ",
	"Jerome Abela",
	"Lauri Alanko",
	"Seth Alves",
	"Shawn T. Amundson",
	"Erik Andersen",
	"Jon Anhold",
	"Timur I. Bakeyev",
	/* for languages that can't display aacute, replace '�' with 'a' */
	N_("Szabolcs 'Shooby' B�n"),
	"Martin Baulig",
	"Tom Bech",
	"Andreas Beck",
	"Carlos Amador Bedolla",
	"Martijn van Beers",
	"Frank Belew",
	"Jacob Berkman",
	"Eckehard Berns",
	"Robert Bihlmeyer",
	"Jonathan Blandford",
	"Christopher Blizzard",
	"Jerome Bolliet",
	"Andreas Bolsch",
	"Dario Bressanini",
	"Emmanuel Briot",
	"Marcus Brubaker",
	"Christian Bucher",
	"Dave Camp",
	"Didier Carlier",
	"Anders Carlsson",
	"Chema Celorio",
	"Damon Chaplin",
	"Kevin Charter",
	"Kenneth Rohde Christiansen",
	"Chad Clark",
	"Matthias Clasen",
	"Andrew Clausen",
	"Jeremy Collins",
	"Rusty Conover",
	"Alan Cox",
	"Mark Crichton",
	"Andreas Czechanowski",
	"Dan Damian",
	"Phil Dawes",
	"Fatih Demir",
	"Frederic Devernay",
	"Dietmar Maurer",
	"Feico W. Dillema",
	"Radek Doulik",
	"Tom Dyas",
	"Karl Eichwalder",
	"John Ellis",
	"Arturo Espinosa",
	"Gus Estrella",
	"David Etherton",
	"Marc Ewing",
	N_("Gerg� �rdi"),
	"Peter Fales",
	"Joaquim Fellmann",
	"Dave Finton",
	"Milon Firikis",
	"Raul Perusquia Flores",
	"Lawrence Foard",
	"Jeff Freedman",
	"David Abilleira Freijeiro",
	"Nat Friedman",
	"Jochen Friedrich",
	"Adam Fritzler",
	"Michael Fulbright",
	"Christopher R. Gabriel",
	"Mark Galassi",
	"Tony Gale",
	"Jeff Garzik",
	N_("The Mysterious GEGL"),
	"Tim Gerla",
	"Bjoern Giesler",
	"Dave Glowacki",
	"Scott Goehring",
	"Randy Gordon",
	"Dov Grobgeld",
	"Bertrand Guiheneuf",
	"Alan Aspuru Guzik",
	"Telsa Gwynne",
	"Fredrik Hallenberg",
	"Lars Hamann",
	"Michael Hanni",
	"Raja R Harinath",
	"Peter Hawkins",
	"Scott Heavner",
	"James Henstridge",
	"Richard Hestilow",
	"Iain Holmes",
	"David Huggins-Daines",
	"Richard Hult",
	"Andreas Hyden",
	"Miguel de Icaza",
	"Tim Janik",
	"Stefan Jeske",
	"Michael K. Johnson",
	"Andy Kahn",
	"Sami Kananoja",
	"Michael Kellen",
	"Stephen Kiernan",
	"Spencer Kimball",
	"Peter Kirchgessner",
	"Helmut Koeberle",
	"Alfredo Kojima",
	"Andrew Kuchling",
	"Stephan Kulow",
	"Martynas Kunigelis",
	"Tuomas Kuosmanen",
	"Olof Kylander",
	"Mathieu Lacage", 
	"Francis J. Lacoste",
	"Chris Lahey",
	"Scott Laird",
	"Birger Langkjer",
	"Alexander Larsson",
	"Guillaume Laurent",
	"Michael Lausch",
	"Will LaShell",
	"Jens Lautenbacher",
	"Evan Lawrence",
	"Garrett LeSage",
	"Jason Leach",
	"George Lebl",
	"Elliot Lee",
	"Marc Lehmann",
	"Raph Levien",
	"Matt Loper",
	"Nick Lopez",
	"Dirk Lutjens",
	"Josh MacDonald",
	"Sam Magnuson",
	"Ian Main",
	"Mandrake",
	"Daniel Manrique",
	"Kjartan Maraas",
	"Matthew Marjanovic",
	"Heath Martin",
	"Oliver Maruhn",
	"Dave Mason",
	"James Mastros",
	"Peter Mattis",
	"Gordon Matzigkeit",
	"Justin Maurer",
	"Gregory McLean",
	"Michael Meeks",
	"Federico Mena-Quintero",
	"Christophe Merlet",
	"Christian Meyer",
	"Cesar Miquel",
	"Julian Missig",
	"Eric B. Mitchell",
	"Jaka Mocnik",
	"Paolo Molaro",
	"David Mosberger",
	"Thomas Muldowney",
	/* for languages that can't display ntilde, replace '�' with 'n' */
	N_("Alexandre Mu�iz"),
	N_("Sung-Hyun Nam"),
	"Karl Nelson",
	"Asger Alstrup Nielsen",
	"Eric Nielson",
	/* languages that can't display adiaeresis, replace '�' with 'ae' */
	N_("Martin Norb�ck"),
	"Eskil Olsen",
	"Jimmy Olsen",
	"David Orme",
	"Karl Anders Oygard",
	/* languages that can't display Odiaeresis, replace '�' with 'Oe' */
	N_("Tomas �gren"),
	"Jay Painter",
	"Cameron Parish",
	"Conrad Parker",
	"Stuart Parmenter",
	"Havoc Pennington",
	"Ettore Perazzoli",
	"Ian Peters",
	"Martin Kasper Petersen",
	"Christof Petig",
	"Joe Pfeiffer",
	"Ben Pierce",
	"Chris Pinkham",
	"Dick Porter",
	"Tero Pulkkinen",
	"The Rasterman",
	"Oliver Rauch",
	"Cody Russell",
	"Reklaw",
	"Jens Christian Restemeier",
	"Patrick Reynolds",
	"Robert Richardson",
	"Alex Roberts",
	"Michel Roelofs",
	"Ueli Rutishauser",
	"Lars Rydlinge",
	"Peter Ryland",
	"Bibek Sahu",
	"Pablo Saratxaga",
	N_("Kazuhiro Sasayama"),
	"Carsten Schaar",
	"Franck Schneider",
	"Ingo Schneider",
	"Bernd Schroeder",
	"John Schulien",
	/* for languages that can't display ntilde, replace '�' with 'n' */ 
	N_("I�igo Serna"),
	"Shaleh",
	"Joe \"Harold\" Shaw",
	"Alejandro Aguilar Sierra",
	"Miroslav Silovic",
	"Manish Singh",
	"Timo Sirainen",
	"David F. Skoll",
	"Nuke Skyjumper",
	"Josh Sled",
	"John Slee",
	"Garrett Smith",
	/* not really a person name :) */
	N_("The Squeaky Rubber Gnome"),
	"Maciej Stachowiak",
	"Stalyn",
	"Ben Stern",
	"Micah Stetson",
	"Nathan Carl Summers",
	"Istvan Szekeres",
	"Tristan Tarrant",
	"Anthony Taylor",
	"Owen Taylor",
	"Peter Teichman",
	"Arturo Tena",
	"Kimball Thurston",
	"Chris Toshok",
	"Christoph Toshok",
	"Tom Tromey",
	"Jon Trowbridge",
	"Manish Vachharajani",
	"Neil Vachharajani",
	"Daniel Veillard",
	"Vendu",
	"Andrew Veliath",
	"Marius Vollmer",
	"Shawn Wagner",
	"Matthias Warkus",
	N_("Wanda the GNOME Fish"),
	"Bruno Widmann",
	"Robert Wilhelm",
	"Sebastian Wilhelmi",
	"David Winkler",
	"Jeremy Wise",
	"Roger Wolff",
	"James Youngman",
	"Orest Zborowski",
	"Sascha Ziemann",
	"Michael Zucchi",
	"Jason van Zyl",
	" ",
	N_("... and many more"),
	NULL, NULL
};



