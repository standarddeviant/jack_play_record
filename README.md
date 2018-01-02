# jack_play_record

After looking for a convenient command line tool, that reliably played and recorded 
multichannel wave files in JACK, I came up short.  So I wrote `jack_play_record` to 
fill that niche.  If something robust already exists and I've missed it, please 
email me or file an issue to update the README.

## Getting Started

To build the code, change to the `jack_play_record` directory on a linux system, and run
```
./build.sh
```

That should leave an executable in the same directory named `jack_play_record`

There are examples below, but the help text is pretty straightforward:
```
Usage: jack_play_record [OPTION...] [-p play.wav | -c chans -r rec.wav]
  -h,    print this help text
  -c,    specify the number of channels (required for recording)
  -n,    specify the name of the jack client
  -f,    specify the intended nframes for use with jack server
         note, that this will save on memory, but is unsafe if the
         jack server nframes value is ever increased
  -w,    wait until W ports have been connected before playing or recording
```

If you want to record a four-channel wave file named `sweet_sounds.wav`, where 
the jack client is named `cool_client`, you can issue the following:

```
./jack_play_record -r sweet_sounds.wav -c 4 -n cool_client
```

When that file has finished recording, you can play that same file back in to jack:
```
./jack_play_record -p sweet_sounds.wav -n really_cool_client
```

The order of the command line arguments is irrelevant.


### Prerequisites

The `build.sh` script is very simple, but requires the following libraries
* `libjack`
* `libsndfile`
* `libpthread`

## Authors

* **Dave Crist**

## License

This project is licensed under the Unlicense License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Thanks to the developers of `JACK`, `libsndfile`, `PortAudio` and `gcc`
* Thanks to [Spencer Russell](https://github.com/ssfrr) for introducing me to the convenient `pa_ringbuffer` code

