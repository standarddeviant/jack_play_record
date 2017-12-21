/** @file simple_client.c
 *
 * @brief This simple client demonstrates the most basic features of JACK
 * as they would be used by many applications.
 */

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sndfile.h>

#include <jack/jack.h>

#define JCK_MAX_PORTS (64)
#define JCK_MAX_FRAMES (16384)
jack_port_t *jckin_ports[JCK_MAX_PORTS];
jack_port_t *jckout_ports[JCK_MAX_PORTS];
jack_client_t *client;

const char *PLAY_NAME = "jack_play";
const char *REC_NAME = "jack_record";
// FIXME w/ enum?
#define PLAY_MODE (SFM_READ)
#define REC_MODE (SFM_WRITE)
#define SND_FNAME_SIZE (2048)
#define JACK_NAME_SIZE (2048)
char sndfname[SND_FNAME_SIZE] = {0};
SNDFILE *sndf;
SF_INFO sndfinfo;
int sndmode = PLAY_MODE;
int sndchans = 0;

char jckname[JACK_NAME_SIZE] = {0};

jack_default_audio_sample_t jckbufI[JCK_MAX_PORTS * JCK_MAX_FRAMES]; // Interleaved
// jack_default_audio_sample_t jckbufD[JCK_MAX_PORTS][JCK_MAX_FRAMES];  // De-interleaved
// jack_default_audio_sample_t jckbufD1[JCK_MAX_FRAMES];  // De-interleaved

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int
process (jack_nframes_t nframes, void *arg)
{
    int cnt, cidx, sidx;
	// jack_default_audio_sample_t *in, *out;
    if(sndmode == PLAY_MODE) {
        // read data from sndf in to interleaved buffer
        cnt = sf_readf_float(sndf, &(jckbufI[0]), nframes*sndchans);
        if(cnt < nframes ){
            sf_seek(sndf, 0, SEEK_SET); // rewind to beginning of file
            int cnt2 = sf_readf_float(sndf, &(jckbufI[cnt*sndchans]),
                (nframes-cnt)*sndfinfo.channels);
            if(cnt + cnt2 != nframes) {
                printf("This is bad, almost certainly a bug...\n");
                // this should not happen, FIXME and error out
            }
        }
        // get jack buffers as needed, and write directly in to those buffers
        for(cidx=0; cidx<sndchans; cidx++) {
            jack_default_audio_sample_t *jckbuf = jack_port_get_buffer(jckout_ports[cidx], nframes);
            for(sidx=0; sidx<nframes; sidx++) {
                *(jckbuf++) = jckbufI[(sidx*sndchans) + cidx];
            }
        }
    }
    else if(sndmode == REC_MODE){
        printf("Implement me and fix me!\n");
    }

	return 0;      
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	// const char *client_name = PLAY_NAME;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

    int cidx, sidx;
	char c;
	while ((c = getopt (argc, argv, "prcnh")) != -1)
    switch (c)
		{
		case 'p':
			sndmode = PLAY_MODE;
			snprintf(sndfname, SND_FNAME_SIZE, "%s", optarg);
        	break;
      	case 'r':
			sndmode = REC_MODE;
			snprintf(sndfname, SND_FNAME_SIZE, "%s", optarg);
        	break;
      	case 'c':
			sndchans = atoi(optarg);
            break;
        case 'n':
            snprintf(jckname, JACK_NAME_SIZE, "%s", optarg);
            break;
        case 'h':
            printf("Usage: jack_play_record [OPTION...] [-p play.wav | -c chans -r rec.wav]\n");
            printf("  -h,           print this help text\n");
            printf("  -c,           specify the number of channels (required for recording)\n");
            printf("  -n, ")
            break;
		// case '?':
		// 	if (optopt == 'c')
		// 		fprintf (stderr, "Option -%c requires an argument.\n", optopt);
		// 	else if (isprint (optopt))
		// 		fprintf (stderr, "Unknown option `-%c'.\n", optopt);
		// 	else
		// 		fprintf (stderr,
		// 				"Unknown option character `\\x%x'.\n",
		// 				optopt);
		// 	return 1;
		default:
    	    abort ();
	}

    /* ensure there's a reasonable jack client name if not already set */
    if( jckname[0] == 0 ) {
        snprintf(jckname, JACK_NAME_SIZE, "%s", \
            (sndmode==PLAY_MODE) ? (PLAY_NAME) : (REC_NAME)) ;
    }

	/* open a client connection to the JACK server */
	client = jack_client_open(jckname, options, &status, server_name);
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
		snprintf(jckname, JACK_NAME_SIZE, "%s", jack_get_client_name(client));
		fprintf(stderr, "unique name `%s' assigned\n", &(jckname[0]));
	}

    /* with an unconfigured jack client, we can do some sndfile prep, like get the sample rate*/
    if( sndmode == PLAY_MODE ){
        sndfinfo.format = 0;
        sndf = sf_open((const char *)sndfname, sndmode, &sndfinfo);
        sndchans = sndfinfo.channels;
    }
    else if(sndmode == REC_MODE ){
        /* if recording, error out if channels is not specified */
        if( sndchans <= 0 || sndchans > 1024 ) {
            printf("\nFor recording, number of channels must be specified with the -c option.  Here is an example:\n");
            printf("    jack_play_record -r file_to_write_to.wav -c 4\n");
            exit(1);
        }
        sndfinfo.samplerate = jack_get_sample_rate(client);
        sndfinfo.channels = sndchans;
        sndfinfo.format = SF_FORMAT_WAV;
        sf_open((const char *)sndfname, sndmode, &sndfinfo);
    }

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate. 
	 */

	printf ("engine sample rate: %" PRIu32 "\n",
		jack_get_sample_rate (client));

	/* create two ports */
    for(cidx=0; cidx<sndchans; cidx++) {
        if(sndmode == PLAY_MODE){
            jckout_ports[cidx] = jack_port_register (client, "output",
                            JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsOutput, 0);
        }
        else if(sndmode == REC_MODE) {
            jckin_ports[cidx] = jack_port_register (client, "input",
                            JACK_DEFAULT_AUDIO_TYPE,
                            JackPortIsInput, 0);
        }
        // else {} , FIXME
    }
    
    // add error case
    // else{
    // }

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

    /* FIXME add command line arg to auto-connect to a block... */
	// ports = jack_get_ports (client, NULL, NULL,
	// 			JackPortIsPhysical|JackPortIsOutput);
	// if (ports == NULL) {
	// 	fprintf(stderr, "no physical capture ports\n");
	// 	exit (1);
	// }

	// if (jack_connect (client, ports[0], jack_port_name (input_port))) {
	// 	fprintf (stderr, "cannot connect input ports\n");
	// }

	// free (ports);
	
	// ports = jack_get_ports (client, NULL, NULL,
	// 			JackPortIsPhysical|JackPortIsInput);
	// if (ports == NULL) {
	// 	fprintf(stderr, "no physical playback ports\n");
	// 	exit (1);
	// }

	// if (jack_connect (client, jack_port_name (output_port), ports[0])) {
	// 	fprintf (stderr, "cannot connect output ports\n");
	// }

	// free (ports);

	/* keep running until stopped by the user */

	sleep (-1);

	/* this is never reached but if the program
	   had some other way to exit besides being killed,
	   they would be important to call.
	*/

	jack_client_close (client);
	exit (0);
}
