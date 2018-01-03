/** @file simple_client.c
 *
 * @brief This simple client demonstrates the most basic features of JACK
 * as they would be used by many applications.
 */

// "standard" libraries
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// libraries/code that require building/linking
#include <jack/jack.h>

#define JACK_GAIN_MAX_PORTS (64)
#define JACK_GAIN_MAX_FRAMES (16384)
jack_port_t *jackin_ports[JACK_GAIN_MAX_PORTS];
jack_port_t *jackout_ports[JACK_GAIN_MAX_PORTS];
jack_client_t *client;

#define JACK_CLIENT_NAME_SIZE (2048)
#define JACK_PORT_NAME_SIZE (2048)
jack_default_audio_sample_t db_gain = 0.0;
jack_default_audio_sample_t linear_gain = 1.0;
int jackchans = 0;
char jackname[JACK_CLIENT_NAME_SIZE] = {0};

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int
jack_process (jack_nframes_t nframes, void *arg)
{
    int cidx;
    jack_nframes_t fidx;
    jack_default_audio_sample_t *jackbufIN;
    jack_default_audio_sample_t *jackbufOUT;
    
    // silence compiler
    arg = arg;
    
    for(cidx=0; cidx<jackchans; cidx++) {
        jackbufIN = jack_port_get_buffer(jackin_ports[cidx], nframes);
        jackbufOUT = jack_port_get_buffer(jackout_ports[cidx], nframes);
        for(fidx=0; fidx<nframes; fidx++) {
            *(jackbufOUT++) = *(jackbufIN++) * linear_gain;
        }
    } // end PLAY_MODE

    return 0;
}




/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
    arg=arg; /* silence compiler */
    exit (1);
}

void usage(void) {
    printf("\n\n");
    printf("Usage: jack_gain [OPTION...] -c chans\n");
    printf("  -h,    print this help text\n");
    printf("  -c,    specify the number of channels\n");
    printf("  -d,    specify the dB gain\n");
    printf("  -l,    specify the linear gain\n");
    printf("  -n,    specify the name of the jack client\n");
    printf("\n\n");
}

void fyi(void) {
    printf("\nINFO: Attempting to run jack_gain\n    where\n    \ndb_gain=%.2f, linear_gain=%.2f\n    channels=%d, and \n    client-name='%s'\n\n",
            db_gain, linear_gain, jackchans, jackname);
}

int main (int argc, char *argv[])
{
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    int cidx, c;

    char portname[JACK_PORT_NAME_SIZE] = {0};

    while ((c = getopt (argc, argv, "c:d:l:n:h")) != -1)
    switch (c)
        {
      	case 'c':
            jackchans = atoi(optarg);
            break;
      	case 'd':
            db_gain = (jack_default_audio_sample_t)(atof(optarg));
            linear_gain = (jack_default_audio_sample_t)(pow(10.0f, db_gain / 20.0f));
            break;
      	case 'l':
            linear_gain = (jack_default_audio_sample_t)(atof(optarg));
            db_gain = (jack_default_audio_sample_t)(20.0 * log10(linear_gain));
            break;
        case 'n':
            snprintf(jackname, JACK_CLIENT_NAME_SIZE, "%s", optarg);
            break;
        case 'h':
            usage();
            return 0;
        default:
            abort ();
    }

    /* after parsing args, if jackchans == 0, then just print usage */
    if(0 == jackchans) {
        usage();
        return 0;
    }

    /* ensure there's a reasonable jack client name if not already set */
    if( jackname[0] == 0 ) {
        snprintf(jackname, JACK_CLIENT_NAME_SIZE, "%s", "jack_gain");
    }

    /* let user know what settings have been parsed */
    fyi();

	/* open a client connection to the JACK server */
	client = jack_client_open(jackname, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		snprintf(jackname, JACK_CLIENT_NAME_SIZE, "%s", jack_get_client_name(client));
		fprintf(stderr, "unique name `%s' assigned\n", &(jackname[0]));
	}

    /* tell the JACK server to call `process()' whenever
        there is work to be done.
    */
    jack_set_process_callback (client, jack_process, 0);

    /* tell the JACK server to call `jack_shutdown()' if
        it ever shuts down, either entirely, or if it
        just decides to stop calling us.
    */

    jack_on_shutdown (client, jack_shutdown, 0);

    /* FIXME, throw error if file sample rate and jack sample rate are different */

    /* create jack ports */
    for(cidx=0; cidx<jackchans; cidx++) {
        snprintf(portname, JACK_PORT_NAME_SIZE, "in_%02d", cidx+1);
        jackin_ports[cidx] = jack_port_register (client, portname,
                JACK_DEFAULT_AUDIO_TYPE,
                JackPortIsInput, 0);

        snprintf(portname, JACK_PORT_NAME_SIZE, "out_%02d", cidx+1);
        jackout_ports[cidx] = jack_port_register(client, portname,                    
                JACK_DEFAULT_AUDIO_TYPE,
                JackPortIsOutput, 0);
    }

    /* Tell the JACK server that we are ready to roll.  Our
    * process() callback will start running now. */

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        exit (1);
    }

    sleep (-1);

    /* this is never reached but if the program
        had some other way to exit besides being killed,
        they would be important to call.
    */

    jack_client_close (client);
    exit (0);
}
