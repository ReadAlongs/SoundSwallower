HUB4 8kHz US English Acoustic Model
===================================

This is the US English acoustic model from PocketSphinx, trained on
HUB4 broadcast news data, downsampled to 8kHz, with no VAD applied.
It uses the old "semi-continuous" acoustic modeling using 256 shared
Gaussian mixture models and four separate feature streams.

Because of the narrow bandwidth and small model, it is not expected to
be very accurate, but it should work well for simple applications, and
should be robust to noisy or band-limited (e.g. telephone) data.
