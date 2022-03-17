// Originally written by and
// Copyright © 2013-2017 Sylvain Chevalier
// MIT license, see LICENSE for details

#include "psRecognizer.h"

namespace pocketsphinxjs {
  Recognizer::Recognizer(): is_fsg(true), is_recording(false), current_hyp("") {
    Config c;
    if (init(c) != SUCCESS) cleanup();
  }

  Recognizer::Recognizer(const Config& config) : is_fsg(true), is_recording(false), current_hyp("") {
    if (init(config) != SUCCESS) cleanup();
  }

  ReturnType Recognizer::reInit(const Config& config) {
    ReturnType r = init(config);
    if (r != SUCCESS) cleanup();
    return r;
  }

  ReturnType Recognizer::addWords(const std::vector<Word>& words) {
    if (decoder == NULL) return BAD_STATE;
    for (int i=0 ; i<words.size() ; ++i) {
      // This case is not properly handeled by ps_add_word, so we treat it separately
      if (words.at(i).pronunciation.size() == 0) return RUNTIME_ERROR;
      if (ps_add_word(decoder, words.at(i).word.c_str(), words.at(i).pronunciation.c_str(), 1) < 0) return RUNTIME_ERROR;
    }
    return SUCCESS;
  }

  ReturnType Recognizer::setGrammar(const Grammar& grammar) {
    if (decoder == NULL) return BAD_STATE;
    current_grammar = fsg_model_init("_default", logmath, 1.0, grammar.numStates);
    if (current_grammar == NULL)
      return RUNTIME_ERROR;
    current_grammar->start_state = grammar.start;
    current_grammar->final_state = grammar.end;
    for (int i=0;i<grammar.transitions.size();i++) {
      if ((grammar.transitions.at(i).word.size() > 0) && (ps_lookup_word(decoder, grammar.transitions.at(i).word.c_str())))
	fsg_model_trans_add(current_grammar, grammar.transitions.at(i).from, grammar.transitions.at(i).to, grammar.transitions.at(i).logp, fsg_model_word_add(current_grammar, grammar.transitions.at(i).word.c_str()));
      else
	fsg_model_null_trans_add(current_grammar, grammar.transitions.at(i).from, grammar.transitions.at(i).to, grammar.transitions.at(i).logp);
    }
    fsg_model_add_silence(current_grammar, "<sil>", -1, 1.0);

    if(ps_set_fsg(decoder, "_default", current_grammar)) {
      return RUNTIME_ERROR;
    }
    return SUCCESS;
  }


  ReturnType Recognizer::start() {
    if ((decoder == NULL) || (is_recording)) return BAD_STATE;
    if ((ps_start_utt(decoder) < 0)) {
      return RUNTIME_ERROR;
    }
    current_hyp = "";
    is_recording = true;
    return SUCCESS;
  }

  ReturnType Recognizer::stop() {
    if ((decoder == NULL) || (!is_recording)) return BAD_STATE;
    if (ps_end_utt(decoder) < 0) {
      return RUNTIME_ERROR;
    }
    const char* h = ps_get_hyp(decoder, NULL);
    current_hyp = (h == NULL) ? "" : h;
    is_recording = false;
    return SUCCESS;
  }

  ReturnType Recognizer::process(const std::vector<int16_t>& buffer) {
    if ((decoder == NULL) || (!is_recording)) return BAD_STATE;
    if (buffer.size() == 0)
      return RUNTIME_ERROR;
    ps_process_raw(decoder, (short int *) &buffer[0], buffer.size(), 0, 0);
    const char* h = ps_get_hyp(decoder, NULL);
    current_hyp = (h == NULL) ? "" : h;
    return SUCCESS;
  }

  std::string Recognizer::lookupWord(const std::string& word) {
    std::string output = "";
    if (word.size() > 0) {
      char * result = ps_lookup_word(decoder, word.c_str());
      if (result != NULL)
	output = result;
    }
    return output;
  }

  Recognizer::~Recognizer() {
    cleanup();
  }

  std::string Recognizer::getHyp() {
    return current_hyp;
  }

  ReturnType Recognizer::getHypseg(Segmentation& seg) {
    if (decoder == NULL) return BAD_STATE;
    seg.clear();
    int32 sfh=0, efh=0;
    int32 ascr=0, lscr=0, lback=0;
    std::string hseg;
    ps_seg_t *itor = ps_seg_iter(decoder);
    while (itor) {
      SegItem segItem;
      segItem.word = ps_seg_word(itor);
      ps_seg_frames(itor, &sfh, &efh);
      ps_seg_prob(itor, &ascr, &lscr, &lback);
      segItem.start = sfh;
      segItem.end = efh;
      segItem.ascr = ascr;
      segItem.lscr = lscr;
      seg.push_back(segItem);
      itor = ps_seg_next(itor);
    }
    return SUCCESS;
  }

  void Recognizer::cleanup() {
    if (decoder) ps_free(decoder);
    if (logmath) logmath_free(logmath);
    decoder = NULL;
    logmath = NULL;
  }

  ReturnType Recognizer::init(const Config& config) {
    const arg_t cont_args_def[] = {
      POCKETSPHINX_OPTIONS,
      { "-time",
	ARG_BOOLEAN,
	"no",
	"Print word times in file transcription." },
      CMDLN_EMPTY_OPTION
    };
    StringsMapType parameters;
    for (int i=0 ; i< config.size() ; ++i)
      parameters[config[i].key] = config[i].value;
    
    if (parameters.find("-hmm") == parameters.end())
      parameters["-hmm"] = default_acoustic_model;
    if (parameters.find("-bestpath") == parameters.end())
      parameters["-bestpath"] = "yes";
    // No longer used, but we will set them just in case
    if (parameters.find("-remove_noise") == parameters.end())
      parameters["-remove_noise"] = "no";
    if (parameters.find("-remove_silence") == parameters.end())
      parameters["-remove_silence"] = "no";

    int argc = 2 * parameters.size();
    char ** argv = new char*[argc];
    int index = 0;
    for (StringsMapIterator i = parameters.begin() ; i != parameters.end(); ++i) {
      argv[index++] = (char*) i->first.c_str();
      argv[index++] = (char*) i->second.c_str();
    }

    cmd_ln_t * cmd_line = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, FALSE);
    if (cmd_line == NULL) {
      delete [] argv;
      return RUNTIME_ERROR;
    }
    decoder = ps_init(cmd_line);
    delete [] argv;
    if (decoder == NULL) {
      return RUNTIME_ERROR;
    }
    logmath = logmath_init(1.0001, 0, 0);
    if (logmath == NULL) {
      return RUNTIME_ERROR;
    }
    return SUCCESS;
  }
} // namespace pocketsphinxjs
