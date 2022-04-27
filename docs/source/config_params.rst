Configuration parameters
========================

These are the parameters currently recognized by
`soundswallower.Config` and `soundswallower.Decoder` along with their
default values.  The configuration mechanism, along with these
parameters, may change in a subsequent release of SoundSwallower.

.. method:: Config(*args, **kwargs)

   Create a SoundSwallower configuration.  This constructor can be
   called with a list of arguments corresponding to a command-line, in
   which case the parameter names should be prefixed with a '-'.
   Otherwise, pass the keyword arguments described below.  For
   example, the following invocations are equivalent::

        config = Config("-hmm", "path/to/things", "-dict", "my.dict")
        config = Config(hmm="path/to/things", dict="my.dict")

   The same keyword arguments can also be passed directly to the
   constructor for `soundswallower.Decoder`.

   :keyword str dict: Main pronunciation dictionary (lexicon) input file
   :keyword str hmm: Directory containing acoustic model files.
   :keyword str logfn: File to write log messages in
   :keyword str fsg: Sphinx format finite state grammar file
   :keyword str jsgf: JSGF grammar file
   :keyword str toprule: Start rule for JSGF (first public rule is default)
   :keyword str fdict: Noise word pronunciation dictionary input file
   :keyword bool dictcase: Dictionary is case sensitive (NOTE: case insensitivity applies to ASCII characters only), defaults to ``False``
   :keyword float beam: Beam width applied to every frame in Viterbi search (smaller values mean wider beam), defaults to ``1e-48``
   :keyword float wbeam: Beam width applied to word exits, defaults to ``7e-29``
   :keyword float pbeam: Beam width applied to phone transitions, defaults to ``1e-48``
   :keyword float samprate: Sampling rate, defaults to ``16000.0``
   :keyword int nfft: Size of FFT, defaults to ``512``
   :keyword str featparams: File containing feature extraction parameters.
   :keyword str mdef: Model definition input file
   :keyword str senmgau: Senone to codebook mapping input file (usually not needed)
   :keyword str tmat: HMM state transition matrix input file
   :keyword float tmatfloor: HMM state transition probability floor (applied to -tmat file), defaults to ``0.0001``
   :keyword str mean: Mixture gaussian means input file
   :keyword str var: Mixture gaussian variances input file
   :keyword float varfloor: Mixture gaussian variance floor (applied to data from -var file), defaults to ``0.0001``
   :keyword str mixw: Senone mixture weights input file (uncompressed)
   :keyword float mixwfloor: Senone mixture weights floor (applied to data from -mixw file), defaults to ``1e-07``
   :keyword int aw: Inverse weight applied to acoustic scores., defaults to ``1``
   :keyword str sendump: Senone dump (compressed mixture weights) input file
   :keyword str mllr: MLLR transformation to apply to means and variances
   :keyword bool mmap: Use memory-mapped I/O (if possible) for model files, defaults to ``True``
   :keyword int ds: Frame GMM computation downsampling ratio, defaults to ``1``
   :keyword int topn: Maximum number of top Gaussians to use in scoring., defaults to ``4``
   :keyword str topn_beam: Beam width used to determine top-N Gaussians (or a list, per-feature), defaults to ``0``
   :keyword float logbase: Base in which all log-likelihoods calculated, defaults to ``1.0001``
   :keyword bool compallsen: Compute all senone scores in every frame (can be faster when there are many senones), defaults to ``False``
   :keyword bool bestpath: Run bestpath (Dijkstra) search over word lattice (3rd pass), defaults to ``True``
   :keyword bool backtrace: Print results and backtraces to log., defaults to ``False``
   :keyword int maxhmmpf: Maximum number of active HMMs to maintain at each frame (or -1 for no pruning), defaults to ``30000``
   :keyword float lw: Language model probability weight, defaults to ``6.5``
   :keyword float ascale: Inverse of acoustic model scale for confidence score calculation, defaults to ``20.0``
   :keyword float wip: Word insertion penalty, defaults to ``0.65``
   :keyword float pip: Phone insertion penalty, defaults to ``1.0``
   :keyword float silprob: Silence word transition probability, defaults to ``0.005``
   :keyword float fillprob: Filler word transition probability, defaults to ``1e-08``
   :keyword bool fsgusealtpron: Add alternate pronunciations to FSG, defaults to ``True``
   :keyword bool fsgusefiller: Insert filler words at each state., defaults to ``True``
   :keyword str mfclogdir: Directory to log feature files to
   :keyword str rawlogdir: Directory to log raw audio files to
   :keyword str senlogdir: Directory to log senone score files to
   :keyword bool logspec: Write out logspectral files instead of cepstra, defaults to ``False``
   :keyword bool smoothspec: Write out cepstral-smoothed logspectral files, defaults to ``False``
   :keyword str transform: Which type of transform to use to calculate cepstra (legacy, dct, or htk), defaults to ``legacy``
   :keyword float alpha: Preemphasis parameter, defaults to ``0.97``
   :keyword int frate: Frame rate, defaults to ``100``
   :keyword float wlen: Hamming window length, defaults to ``0.025625``
   :keyword int nfilt: Number of filter banks, defaults to ``40``
   :keyword float lowerf: Lower edge of filters, defaults to ``133.33334``
   :keyword float upperf: Upper edge of filters, defaults to ``6855.4976``
   :keyword bool unit_area: Normalize mel filters to unit area, defaults to ``True``
   :keyword bool round_filters: Round mel filter frequencies to DFT points, defaults to ``True``
   :keyword int ncep: Number of cep coefficients, defaults to ``13``
   :keyword bool doublebw: Use double bandwidth filters (same center freq), defaults to ``False``
   :keyword int lifter: Length of sin-curve for liftering, or 0 for no liftering., defaults to ``0``
   :keyword str input_endian: Endianness of input data, big or little, ignored if NIST or MS Wav, defaults to ``little``
   :keyword str warp_type: Warping function type (or shape), defaults to ``inverse_linear``
   :keyword str warp_params: Parameters defining the warping function
   :keyword bool dither: Add 1/2-bit noise, defaults to ``False``
   :keyword int seed: Seed for random number generator; if less than zero, pick our own, defaults to ``-1``
   :keyword bool remove_dc: Remove DC offset from each frame, defaults to ``False``
   :keyword bool remove_noise: UNSUPPORTED option, do not use, defaults to ``False``
   :keyword bool remove_silence: UNSUPPORTED option, do not use, defaults to ``False``
   :keyword bool verbose: Show input filenames, defaults to ``False``
   :keyword str feat: Feature stream type, depends on the acoustic model, defaults to ``1s_c_d_dd``
   :keyword int ceplen: Number of components in the input feature vector, defaults to ``13``
   :keyword str cmn: Cepstral mean normalization scheme ('live', 'batch', or 'none'), defaults to ``live``
   :keyword str cmninit: Initial values (comma-separated) for cepstral mean when 'live' is used, defaults to ``40,3,-1``
   :keyword bool varnorm: Variance normalize each utterance (only if CMN == current), defaults to ``False``
   :keyword str lda: File containing transformation matrix to be applied to features (single-stream features only)
   :keyword int ldadim: Dimensionality of output of feature transformation (0 to use entire matrix), defaults to ``0``
   :keyword str svspec: Subvector specification (e.g., 24,0-11/25,12-23/26-38 or 0-12/13-25/26-38)
