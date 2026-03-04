/* duti: set default handlers for document types based on a settings file. */

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "handler.h"

extern char		*duti_version;
extern int		nroles;
int			verbose = 0;

struct roles		rtm[] = {
    { "none",   kLSRolesNone },
    { "viewer", kLSRolesViewer },
    { "editor", kLSRolesEditor },
    { "shell",  kLSRolesShell },
    { "all",    kLSRolesAll },
};

    static void
usage( const char *prog )
{
    fprintf( stderr,
	"duti — set default handlers for document types on macOS\n"
	"\n"
	"Usage:\n"
	"  %s -s bundle_id|app_path { uti | extension | MIME | url_scheme } [ role ]\n"
	"  %s -x extension\n"
	"  %s -d uti\n"
	"  %s -l uti\n"
	"  %s [ settings_path ]\n"
	"\n"
	"Options:\n"
	"  -s               Set default handler by bundle ID, or by app path\n"
	"                   when multiple apps share the same bundle ID\n"
	"  -x extension     Show default app for file extension\n"
	"  -d uti           Show default handler for UTI\n"
	"  -l uti           List all handlers for UTI\n"
	"  -e extension     Show UTI declarations for extension\n"
	"  -u uti           Show UTI declaration details\n"
	"  -v, --verbose    Enable verbose output\n"
	"  -V, --version    Print version and exit\n"
	"  -h, --help       Show this help message\n"
	"\n"
	"Roles: none, viewer, editor, shell, all\n"
	"\n"
	"Examples:\n"
	"  %s -s com.apple.Safari public.html all\n"
	"  %s -s /Applications/MyApp.app .txt editor  # useful when apps share same bundle ID\n"
	"  %s -x jpg\n",
	prog, prog, prog, prog, prog, prog, prog, prog );
}

    int
main( int ac, char *av[] )
{
    struct stat		st;
    int			c, err = 0;
    int			set = 0;
    int			( *handler_f )( char * );
    char		*path = NULL;
    char		*p;

    extern int		optind;
    extern char		*optarg;

    static struct option long_options[] = {
	{ "help",    no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ "verbose", no_argument, NULL, 'v' },
	{ NULL,      0,           NULL,  0  },
    };

    while (( c = getopt_long( ac, av, "d:e:l:hsu:Vvx:",
		    long_options, NULL )) != -1 ) {
	switch ( c ) {
	case 'd':	/* show default handler for UTI */
	    return( uti_handler_show( optarg, 0 ));

	case 'e':	/* UTI declarations for extension */
		return( duti_utis_for_extension( optarg ));

	case 'h':	/* help */
	    usage( av[ 0 ] );
	    exit( 0 );

	case 'l':	/* list all handlers for UTI */
	    return( uti_handler_show( optarg, 1 ));

	case 's':	/* set handler */
	    set = 1;
	    break;

	case 'u':	/* UTI declarations */
		return( duti_utis( optarg ));

	case 'V':	/* version */
#ifdef BUILD_TIMESTAMP
	    printf( "%s (built %s)\n", duti_version, BUILD_TIMESTAMP );
#else
	    printf( "%s\n", duti_version );
#endif
	    exit( 0 );

	case 'v':	/* verbose */
	    verbose = 1;
	    break;

	case 'x':	/* info for extension */
	    return( duti_default_app_for_extension( optarg ));

	default:
	    usage( av[ 0 ] );
	    exit( 1 );
	}
    }

    nroles = sizeof( rtm ) / sizeof( rtm[ 0 ] );

    switch (( ac - optind )) {
    case 0 :	/* read from stdin */
	if ( set ) {
	    err++;
	}
	break;

    case 1 :	/* read from file or directory */
	if ( set ) {
	    err++;
	}
	path = av[ optind ];
	break;

    case 2 :	/* set URI handler */
	if ( set ) {
	    return( duti_handler_set( av[ optind ],
		    av[ optind + 1 ], NULL ));
	}
	/* this fallthrough works because set == 0 */

    case 3 :	/* set UTI handler */
	if ( set ) {
	    return( duti_handler_set( av[ optind ],
		    av[ optind + 1 ], av[ optind + 2 ] ));
	}
	/* fallthrough to error */

    default :	/* error */
	err++;
	break;
    }

    if ( err ) {
	usage( av[ 0 ] );
	exit( 1 );
    }

    /* by default, read from a FILE stream */
    handler_f = fsethandler;

    if ( path ) {
	if ( stat( path, &st ) != 0 ) {
	    fprintf( stderr, "stat %s: %s\n", path, strerror( errno ));
	    exit( 2 );
	}
	switch ( st.st_mode & S_IFMT ) {
	case S_IFDIR:	/* directory of settings files */
	    handler_f = dirsethandler;
	    break;

	case S_IFREG:	/* settings file or plist */
	    if (( p = strrchr( path, '.' )) != NULL ) {
		p++;
		if ( strcmp( p, "plist" ) == 0 ) {
		    handler_f = psethandler;
		}
	    }
	    break;

	default:
	    fprintf( stderr, "%s: not a supported settings path\n", path );
	    exit( 1 );
	}
    }

    return( handler_f( path ));
}
