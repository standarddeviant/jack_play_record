# a proper makefile would be nice, but this is functional



gcc -Wall -Wextra -Wunused         \
    -o jack_play_record            \
    jack_play_record.c             \
    pa_ringbuffer/pa_ringbuffer.c  \
    -I ./pa_ringbuffer/            \
    -ljack -lsndfile -lpthread

gcc -Wall -Wextra -Wunused \
    -o jack_gain           \
    jack_gain.c            \
    -ljack -lm


