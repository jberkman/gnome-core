%{

#include "xlockmore.h"

static XLockMode *curMode;
static XLockModePar *curPar;
static gchar *modeName,
              *parName;
static gint optType;

%}

MODE	"-"|[a-z]|[A-Z]|[0-9]

%x modpar mod par parval optval
%x m1 m2 m3 m4 m5 m6 m7

%%

^XLock\.			BEGIN (modpar);

<modpar>{MODE}+/\.		{
	if (!g_hash_table_lookup (XLockMore::xlockmore->modes, yytext)) {
		// printf ("mode: %s\n", yytext);
		curMode = new XLockMode (XLockMore::xlockmore, yytext, "");
		XLockMore::xlockmore->addMode (curMode);
	}
	BEGIN (par);
}
<modpar>{MODE}+/:		{
	// printf ("option: %s\n", yytext);
	BEGIN (optval);
}

<par>\.{MODE}+			{
	// printf ("par: %s\n", yytext+1);
	parName	= g_strdup (yytext +1);
	BEGIN (parval);
}

<parval>:" "*
<parval>([Tt][Rr][Uu][Ee])|([Ff][Aa][Ll][Ss][Ee])	{
	curMode->addPar (new XLockModePar (parName, "", XLMP_BOOL_ARG, yytext));

	g_free (parName);
	BEGIN (INITIAL);
}
<parval>{MODE}+			{
	curMode->addPar (new XLockModePar (parName, "", XLMP_STRING_ARG, yytext));
	g_free (parName);
	BEGIN (INITIAL);
}

<optval>:" "*
<optval>{MODE}+			{
	BEGIN (INITIAL);
}

<m1>" "*			BEGIN (m2);
<m2>{MODE}+			{
	// printf ("mode: %s\n", yytext);
	BEGIN (m3);
	modeName = g_strdup (yytext);
}

<m3>\ *				BEGIN (m5);
<m5>.*				{
	// printf ("comment: %s\n", yytext);
	BEGIN (m1);
	curMode = new XLockMode (XLockMore::xlockmore, modeName, yytext);
	g_free (modeName);

	XLockMore::xlockmore->addMode (curMode);
}

<m2>\-\/\+			optType = XLMP_BOOL_ARG; BEGIN (m7);
<m2>\-				optType = XLMP_BOOL_ARG; BEGIN (m7);

<m7>{MODE}+			{
	// printf ("option: %s\n", yytext);
	BEGIN (m4);
	parName = g_strdup (yytext);
}

<m4>" "{MODE}+			{
	// printf ("par: %s\n", yytext+1);
	BEGIN (m6);
	/* !!! here also set optType */
}

<m4>" "+			BEGIN (m6);
<m6>.*				{
	// printf ("comment: %s\n", yytext);
	BEGIN (m1);
	curMode->addPar (new XLockModePar (parName, g_strdup (yytext), optType, ""));
	g_free (parName);
}

%%

int yywrap () {
	return 1;
}
