define([AC_FIND_PROGRAM],dnl
[if test x$3 = x; then _PATH=$PATH; else _PATH=$3; fi
if test -z "[$]$1"; then
  # Extract the first word of `$2', so it can be a program name with args.
  set dummy $2; word=[$]2
  AC_MSG_CHECKING(for $word)
  IFS="${IFS= 	}"; saveifs="$IFS"; IFS="${IFS}:"
  for dir in $_PATH; do
    test -z "$dir" && dir=.
    if test -f $dir/$word; then
      $1=$dir/$word
      break
    fi
  done
  IFS="$saveifs"
fi
AC_MSG_RESULT([$]$1)
AC_SUBST($1)dnl
])dnl
