#include <gnome.h>

typedef struct {
	const gchar *name;
	const gchar *email;
} author;

/* Please keep this in alphabetical order */
static author authors[] = {
	{ N_("GNOME was brought to you by"), "" },
	{ "", "" },
	{ "Jerome Abela", "" },
	{ "Lauri Alanko", "" },
	{ "Seth Alves", "" },
	{ "Shawn T. Amundson", "" },
	{ "Erik Andersen", "" },
	{ "Jon Anhold", "" },
	{ "Martin Baulig", "<martin@home-of-linux.org>" },
	{ "Tom Bech", "" },
	{ "Andreas Beck", "" },
	{ "Carlos Amador Bedolla", "" },
	{ "Martijn van Beers", "" },
	{ "Frank Belew", "" },
	{ "Jacob Berkman", "<jberkman@andrew.cmu.edu>" },
	{ "Frank Belew", "" },
	{ "Eckehard Berns", "" },
	{ "Robert Bihlmeyer", "" },
	{ "Jonathan Blandford", "<jrb@redhat.com>" },
	{ "Christopher Blizzard", "" },
	{ "Jerome Bolliet", "" },
	{ "Andreas Bolsch", "" },
	{ "Dario Bressanini", "" },
	{ "Emmanuel Briot", "" },
	{ "Marcus Brubaker", "" },
	{ "Christian Bucher", "" },
	{ "Didier Carlier", "" },
	{ "Anders Carlsson", "<andersca@gnu.org>" },
	{ "Damon Chaplin", "" },
	{ "Kevin Charter", "" },
	{ "Kenneth Christi�nsen", "<kenneth@ripen.dk>" },
	{ "Chad Clark", "" },
	{ "Matthias Clasen", "" },
	{ "Andrew Clausen", "" },
	{ "Jeremy Collins", "" },
	{ "Alan Cox", "" },
	{ "Mark Crichton", "" },
	{ "Andreas Czechanowski", "" },
	{ "Phil Dawes", "" },
	{ "Philip Dawes", "" },
	{ "Frederic Devernay", "" },
	{ "Maurer Dietmar", "" },
	{ "Feico W. Dillema", "" },
	{ "Radek Doulik", "" },
	{ "Tom Dyas", "" },
	{ "Karl Eichwalder", "<ke@suse.de>" },
	{ "John Ellis", "" },
	{ "Arturo Espinosa", "" },
	{ "Gus Estrella", "" },
	{ "David Etherton", "" },
	{ "Marc Ewing", "" },
	{ "Peter Fales", "" },
	{ "Dave Finton", "" },
	{ "Milon Firikis", "" },
	{ "Raul Perusquia Flores", "" },
	{ "Lawrence Foard", "" },
	{ "Jeff Freedman", "" },
	{ "David Abilleira Freijeiro", "" },
	{ "Nat Friedman", "" },
	{ "Jochen Friedrich", "" },
	{ "Adam Fritzler", "" },
	{ "Michael Fulbright", "" },
	{ "Mark Galassi", "" },
	{ "Tony Gale", "" },
	{ "Jeff Garzik", "" },
	{ "The Mysterious GEGL", "" },
	{ "Tim Gerla", "<timg@means.net>" },
	{ "Bjoern Giesler", "" },
	{ "Dave Glowacki", "" },
	{ "Scott Goehring", "" },
	{ "Randy Gordon", "" },
	{ "Dov Grobgeld", "" },
	{ "Bertrand Guiheneuf", "<Bertrand.Guiheneuf@aful.org>" },
	{ "Alan Aspuru Guzik", "" },
	{ "Fredrik Hallenberg", "" },
	{ "Lars Hamann", "" },
	{ "Michael Hanni", "" },
	{ "Raja R Harinath", "" },
	{ "Scott Heavner", "" },
	{ "James Henstridge", "<james@daa.com.au>" },
	{ "Richard Hestilow", "" },
	{ "David Huggins-Daines", "" },
	{ "Richard Hult", "<d4hult@dtek.chalmers.se>" },
	{ "Miguel de Icaza", "<miguel@gnu.org>" },
	{ "Szekeres Istvan", "" },
	{ "Tim Janik", "" },
	{ "Stefan Jeske", "" },
	{ "Michael K. Johnson", "" },
	{ "Andy Kahn", "" },
	{ "Sami Kananoja", "" },
	{ "Michael Kellen", "" },
	{ "Stephen Kiernan", "" },
	{ "Spencer Kimball", "" },
	{ "Peter Kirchgessner", "" },
	{ "Helmut Koeberle", "" },
	{ "Alfredo Kojima", "" },
	{ "Andrew Kuchling", "" },
	{ "Stephan Kulow", "" },
	{ "Martynas Kunigelis", "" },
	{ "Tuomas Kuosmanen", "<tigert@gimp.org>" },
	{ "Olof Kylander", "" },
	{ "Francis J. Lacoste", "" },
	{ "Chris Lahey", "" },
	{ "Scott Laird", "" },
	{ "Birger Langkjer", "<birger.langkjer@image.dk>" },
	{ "Alexander Larsson", "" },
	{ "Guillaume Laurent", "" },
	{ "Michael Lausch", "" },
	{ "Will LaShell", "" },
	{ "Jens Lautenbacher", "" },
	{ "Evan Lawrence", "" },
	{ "Garrett LeSage", "" },
	{ "George Lebl", "<jirka@5z.com>" },
	{ "Elliot Lee", "<sopwith@redhat.com>" },
	{ "Marc Lehmann", "" },
	{ "Raph Levien", "" },
	{ "Matt Loper", "" },
	{ "Nick Lopez", "" },
	{ "Dirk Lutjens", "" },
	{ "Josh MacDonald", "" },
	{ "Sam Magnuson", "" },
	{ "Ian Main", "<slow@intergate.bc.ca>" },
	{ "Mandrake", "" },
	{ "Daniel Manrique", "" },
	{ "Kjartan Maraas", "<kmaraas@online.no>" },
	{ "Matthew Marjanovic", "" },
	{ "Heath Martin", "" },
	{ "Oliver Maruhn", "" },
	{ "Dave Mason", "" },
	{ "James Mastros", "" },
	{ "Peter Mattis", "" },
	{ "Gordon Matzigkeit", "" },
	{ "Justin Maurer", "" },
	{ "Gregory McLean", "" },
	{ "Michael Meeks", "<mmeeks@gnu.org>" },
	{ "Federico Mena-Quintero", "<federico@gimp.org>" },
	{ "Cesar Miquel", "" },
	{ "Eric B. Mitchell", "" },
	{ "Jaka Mocnik", "<jaka.mocnik@kiss.uni-lj.si>" },
	{ "Paolo Molaro", "" },
	{ "David Mosberger", "" },
	{ "Thomas Muldowney", "" },
	{ "Sung-Hyun Nam", "" },
	{ "Karl Nelson", "" },
	{ "Asger Alstrup Nielsen", "" },
	{ "Eric Nielson", "" },
	{ "Eskil Olsen", "" },
	{ "Jimmy Olsen", "" },
	{ "David Orme", "" },
	{ "Karl Anders Oygard", "" },
	{ "Tomas �gren", "<stric@ing.umu.se>" },
	{ "Jay Painter", "" },
	{ "Molaro Paolo", "" },
	{ "Cameron Parish", "" },
	{ "Conrad Parker", "" },
	{ "Stuart Parmenter", "" },
	{ "Havoc Pennington", "" },
	{ "Ettore Perazzoli", "<ettore@gnu.org>" },
	{ "Ian Peters", "<itp@gnu.org>" },
	{ "Martin Kasper Petersen", "" },
	{ "Christof Petig", "" },
	{ "Joe Pfeiffer", "" },
	{ "Ben Pierce", "" },
	{ "Chris Pinkham", "" },
	{ "Dick Porter", "" },
	{ "Tero Pulkkinen", "" },
	{ "The Rasterman", "" },
	{ "Oliver Rauch", "" },
	{ "Reklaw", "" },
	{ "Jens Christian Restemeier", "" },
	{ "Patrick Reynolds", "" },
	{ "Robert Richardson", "" },
	{ "Alex Roberts", "" },
	{ "Michel Roelofs", "" },
	{ "Ueli Rutishauser", "" },
	{ "Lars Rydlinge", "" },
	{ "Peter Ryland", "" },
	{ "Bibek Sahu", "" },
	{ "Kazuhiro Sasayama", "" },
	{ "Carsten Schaar", "" },
	{ "Franck Schneider", "" },
	{ "Ingo Schneider", "" },
	{ "Bernd Schroeder", "" },
	{ "John Schulien", "" },
	{ "I�igo Serna", "" },
	{ "Shaleh", "" },
	{ "Alejandro Aguilar Sierra", "" },
	{ "Miroslav Silovic", "" },
	{ "Manish Singh", "" },
	{ "Timo Sirainen", "" },
	{ "David F. Skoll", "" },
	{ "Nuke Skyjumper", "" },
	{ "Josh Sled", "" },
	{ "John Slee", "" },
	{ "Garrett Smith", "" },
	{ "The Squeaky Rubber Gnome", "<squeak>" },
	{ "Maciej Stachowiak", "" },
	{ "Stalyn", "" },
	{ "Ben Stern", "" },
	{ "Micah Stetson", "" },
	{ "Nathan Carl Summers", "" },
	{ "Tristan Tarrant", "" },
	{ "Anthony Taylor", "" },
	{ "Owen Taylor", "<otaylor@redhat.com>" },
	{ "Peter Teichman", "<pat4@acpub.duke.edu>" },
	{ "Arturo Tena", "<arturo@directmail.org>" },
	{ "Kimball Thurston", "" },
	{ "Chris Toshok", "" },
	{ "Christoph Toshok", "" },
	{ "Tom Tromey", "<tromey@cygnus.com>" },
	{ "Jon Trowbridge", "" },
	{ "Manish Vachharajani", "" },
	{ "Neil Vachharajani", "" },
	{ "Daniel Veillard", "" },
	{ "Vendu", "" },
	{ "Andrew Veliath", "" },
	{ "Marius Vollmer", "" },
	{ "Shawn Wagner", "" },
	{ "Matthias Warkus", "<mawa@iname.com>" },
	{ "Wanda the GNOME Fish", "" },
	{ "Bruno Widmann", "" },
	{ "Robert Wilhelm", "" },
	{ "Sebastian Wilhelmi", "" },
	{ "David Winkler", "" },
	{ "Jeremy Wise", "" },
	{ "Roger Wolff", "" },
	{ "James Youngman", "" },
	{ "Orest Zborowski", "" },
	{ "Sascha Ziemann", "" },
	{ "Michael Zucchi", "<zucchi@zedzone.mmc.com.au>" },
	{ "Jason van Zyl", "" },
	{ "", "" },
	{ N_("... and many more"), "" },
	{ NULL, NULL }
};



