dnl Setup macros for stuff used over and over again.
dnl MC_IF_VAR_EQ(environment-variable, value [, equals-action] [, else-action])
AC_DEFUN(MC_IF_VAR_EQ,[
	case "$[$1]" in
	"[$2]"[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])
dnl MC_STR_CONTAINS(src-string, sub-string [, contains-action] [, else-action])
AC_DEFUN(MC_STR_CONTAINS,[
	case "[$1]" in
	*"[$2]"*[)]
		[$3]
		;;
	*[)]
		[$4]
		;;
	esac
])
dnl MC_ADD_TO_VAR(environment-variable, check-string, add-string)
AC_DEFUN(MC_ADD_TO_VAR,[
	MC_STR_CONTAINS($[$1], [$2], [$1]="$[$1]", [$1]="$[$1] [$3]")
])
