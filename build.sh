# a proper makefile would be nice, but this is functional



gcc -o jack_play_record            \
    jack_play_record.c             \
    pa_ringbuffer/pa_ringbuffer.c  \
    -I ./pa_ringbuffer/            \
    -ljack -lsndfile

