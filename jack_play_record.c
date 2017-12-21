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

#define JACK_PLAY_RECORD_MAX_PORTS (64)
#define JACK_PLAY_RECORD_MAX_FRAMES (16384)
jack_port_t *jackin_ports[JACK_PLAY_RECORD_MAX_PORTS];
jack_port_t *jackout_ports[JACK_PLAY_RECORD_MAX_PORTS];
jack_client_t *client;

const char *PLAY_NAME = "jack_play";
const char *REC_NAME = "jack_record";
// FIXME w/ enum?
#define PLAY_MODE (SFM_READ)
#define REC_MODE (SFM_WRITE)
#define SND_FNAME_SIZE (2048)
#define JACK_CLIENT_NAME_SIZE (2048)
#define JACK_PORT_NAME_SIZE (2048)
char sndfname[SND_FNAME_SIZE] = {0};
SNDFILE *sndf;
SF_INFO sndfinfo;
int sndmode = PLAY_MODE;
int sndchans = 0;

char jackname[JACK_CLIENT_NAME_SIZE] = {0};

// Interleaved buffer
jack_default_audio_sample_t jackbufI[JACK_PLAY_RECORD_MAX_PORTS * JACK_PLAY_RECORD_MAX_FRAMES]; 

// jack_default_audio_sample_t jackbufD[JACK_PLAY_RECORD_MAX_PORTS][JACK_PLAY_RECORD_MAX_FRAMES];  // De-interleaved
// jack_default_audio_sample_t jackbufD1[JACK_PLAY_RECORD_MAX_FRAMES];  // De-interleaved

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
    int cnt, cidx, fidx, sidx;
    // jack_default_audio_sample_t *in, *out;
    if(sndmode == PLAY_MODE) {
        // read data from sndf in to interleaved buffer
        cnt = sf_readf_float(sndf, &(jackbufI[0]), nframes*sndchans);
        if(cnt < nframes ){
            sf_seek(sndf, 0, SEEK_SET); // rewind to beginning of file
            int cnt2 = sf_readf_float(sndf, &(jackbufI[cnt*sndchans]),
                (nframes-cnt)*sndfinfo.channels);
            if(cnt + cnt2 != nframes) {
                printf("This is bad, almost certainly a bug...\n");
                // this should not happen, FIXME and error out
            }
        }
        // get jack buffers as needed, and write directly in to those buffers
        for(cidx=0; cidx<sndchans; cidx++) {
            jack_default_audio_sample_t *jackbuf = jack_port_get_buffer(jackout_ports[cidx], nframes);
            for(fidx=0; fidx<nframes; fidx++) {
                *(jackbuf++) = jackbufI[(fidx*sndchans) + cidx];
            }
        }
    }
    else if(sndmode == REC_MODE) {
        // get pointers for all jack port buffers
        jack_default_audio_sample_t *jackbufs[JACK_PLAY_RECORD_MAX_PORTS];
        for(cidx=0; cidx<sndchans; cidx++) {
            jackbufs[cidx] = jack_port_get_buffer(jackin_ports[cidx], nframes);
        }
        
        // write to sndfile one sample at a time
        // set outer loop over frames/samples
        sidx = 0; // use sample index to book-keep current index in to jackbufI
        for(fidx=0; fidx<nframes; fidx++) {
            // set inner loop over channels/jackbufs
            for(cidx=0; cidx<sndchans; cidx++) {
                // this is naive, but might be fast enough
                // the real problem is that this really ought to be threaded...
                // but that means threading in C :-/
                jackbufI[sidx++] = jackbufs[cidx][fidx];
            }
        }

        cnt = sf_writef_float(sndf, &(jackbufI[0]), nframes*sndchans);
        if(cnt != nframes*sndchans) {
            printf("after writing to file, cnt (samples) = %d, but nframes*sndchans (%d * %d) = %d\n", cnt, nframes, sndchans, nframes*sndchans);
        }
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

void usage(void) {
    printf("\n\n");
    printf("Usage: jack_play_record [OPTION...] [-p play.wav | -c chans -r rec.wav]\n");
    printf("  -h,           print this help text\n");
    printf("  -c,           specify the number of channels (required for recording)\n");
    printf("  -n,           specify the name of the jack client\n");
    printf("\n\n");
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
	int c;

    char portname[JACK_PORT_NAME_SIZE] = {0};

	while ((c = getopt (argc, argv, "p:r:c:n:h")) != -1)
    switch (c)
		{
		case 'p':
			sndmode = PLAY_MODE;
			snprintf(sndfname, SND_FNAME_SIZE, "%s", optarg);
            printf("p: sndfname = -->%s<--, sndmode = %d, sndchans = %d\n", sndfname, sndmode, sndchans);
        	break;
      	case 'r':
			sndmode = REC_MODE;
			snprintf(sndfname, SND_FNAME_SIZE, "%s", optarg);
            printf("r: sndfname = -->%s<--, sndmode = %d, sndchans = %d\n", sndfname, sndmode, sndchans);
        	break;
      	case 'c':
			sndchans = atoi(optarg);
            printf("c: sndfname = -->%s<--, sndmode = %d, sndchans = %d\n", sndfname, sndmode, sndchans);
            break;
        case 'n':
            snprintf(jackname, JACK_CLIENT_NAME_SIZE, "%s", optarg);
            printf("n: sndfname = -->%s<--, sndmode = %d, sndchans = %d\n", sndfname, sndmode, sndchans);
            break;
        case 'h':
            usage();
            return 0;
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

    /* after parsing args, if sndfname is empty, then just print usage */
    if(0 == strlen((const char *)sndfname)) {
        usage();
        return 0;
    }

    /* ensure there's a reasonable jack client name if not already set */
    if( jackname[0] == 0 ) {
        snprintf(jackname, JACK_CLIENT_NAME_SIZE, "%s", \
            (sndmode==PLAY_MODE) ? (PLAY_NAME) : (REC_NAME)) ;
    }


    printf("DBG: jackname = %s\n", jackname);

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
        sndfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        sndf = sf_open((const char *)sndfname, sndmode, &sndfinfo);
    }

    int sferr = sf_error(sndf);
    if(sferr) {
        printf("Tried to open %s and obtained this error code from sf_error: %d\n",
                sndfname, sferr);
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
            snprintf(portname, JACK_PORT_NAME_SIZE, "out_%02d", cidx+1);
            jackout_ports[cidx] = jack_port_register(client, portname,                    
                    JACK_DEFAULT_AUDIO_TYPE,
                    JackPortIsOutput, 0);
            printf("jackout_ports[%d] = %p\n", cidx, jackout_ports[cidx]);
        }
        else if(sndmode == REC_MODE) {
            snprintf(portname, JACK_PORT_NAME_SIZE, "in_%02d", cidx+1);
            jackin_ports[cidx] = jack_port_register (client, portname,
                    JACK_DEFAULT_AUDIO_TYPE,
                    JackPortIsInput, 0);
            printf("jackin_ports[%d] = %p\n", cidx, jackin_ports[cidx]);
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
