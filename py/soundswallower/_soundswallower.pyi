from typing import Union, Iterator, Tuple, Optional, Sequence, ClassVar
import soundswallower


class Config:
    def __init__(self, *args, **kwargs) -> None:
        ...

    @staticmethod
    def parse_json(json: Union[bytes, str]) -> "Config":
        ...

    def dumps(self) -> str:
        ...

    def __contains__(self, key: Union[bytes, str]) -> bool:
        ...

    def __getitem__(self, key: Union[bytes, str]) -> Union[str, int, float, bool]:
        ...

    def __setitem__(self, key: Union[bytes, str], val: Union[str, int, float, bool]):
        ...

    def __delitem__(self, key: Union[bytes, str]):
        ...

    def __iter__(self) -> Iterator[str]:
        ...

    def items(self) -> Iterator[Tuple[str, Union[str, int, float, bool]]]:
        ...

    def __len__(self) -> int:
        ...

    def describe(self) -> Iterator[soundswallower.Arg]:
        ...


class FsgModel:
    pass


class Alignment:
    pass


class Decoder:
    config: Config
    cmn: str
    hyp: soundswallower.Hyp
    seg: Iterator[soundswallower.Seg]
    alignment: Alignment
    n_frames: int

    def __init__(self, *args, **kwargs) -> None:
        ...

    @staticmethod
    def create(*args, **kwargs) -> "Decoder":
        ...

    def initialize(self, config: Optional[Config] = ...):
        ...

    def reinit_feat(self, config: Optional[Config] = ...):
        ...

    def update_cmn(self) -> str:
        ...

    def start_utt(self) -> None:
        ...

    def process_raw(
        self,
        data: bytes,
        no_search: bool = ...,
        full_utt: bool = ...,
    ):
        ...

    def end_utt(self) -> None:
        ...

    def add_word(self, word: str, phones: str, update: bool = ...) -> int:
        ...

    def lookup_word(self, word: str) -> int:
        ...

    def read_fsg(self, filename: str) -> FsgModel:
        ...

    def read_jsgf(self, filename: str) -> FsgModel:
        ...

    def create_fsg(
        self,
        name: str,
        start_state: int,
        final_state: int,
        transitions: Sequence[Tuple],
    ) -> FsgModel:
        ...

    def parse_jsgf(
        self, jsgf_string: Union[bytes, str], toprule: Optional[str] = ...
    ) -> FsgModel:
        ...

    def set_fsg(self, fsg: FsgModel):
        ...

    def set_jsgf_file(self, filename: str):
        ...

    def set_jsgf_string(self, jsgf_string: Union[bytes, str]):
        ...

    def decode_file(self, input_file: str) -> Tuple[str, Iterator[soundswallower.Seg]]:
        ...

    def dumps(self, start_time: float = ..., align_level: int = ...) -> str:
        ...

    def set_align_text(self, text: str):
        ...


class Vad:
    LOOSE: ClassVar[int]
    MEDIUM_LOOSE: ClassVar[int]
    MEDIUM_STRICT: ClassVar[int]
    STRICT: ClassVar[int]
    DEFAULT_SAMPLE_RATE: ClassVar[int]
    DEFAULT_FRAME_LENGTH: ClassVar[float]

    frame_bytes: int
    frame_length: float
    sample_rate: int

    def __init__(
        self, mode: int = ..., sample_rate: int = ..., frame_length: float = ...
    ) -> None:
        ...

    def is_speech(self, frame: bytes, sample_rate: int = ...) -> bool:
        ...


class Endpointer:
    DEFAULT_WINDOW: ClassVar[float]
    DEFAULT_RATIO: ClassVar[float]
    frame_bytes: int
    frame_length: float
    sample_rate: int
    in_speech: bool
    speech_start: float
    speech_end: float

    def __init__(
        self,
        window: float = ...,
        ratio: float = ...,
        vad_mode: int = ...,
        sample_rate: int = ...,
        frame_length: float = ...,
    ) -> None:
        ...

    def process(self, frame: bytes) -> Optional[bytes]:
        ...

    def end_stream(self, frame: bytes) -> Optional[bytes]:
        ...


class AlignmentEntry:
    start: int
    duration: int
    score: int
    name: str

    def __iter__(self) -> Iterator["AlignmentEntry"]:
        ...


class Alignment:
    def __iter__(self) -> Iterator[AlignmentEntry]:
        ...

    def words(self) -> Iterator[AlignmentEntry]:
        ...

    def phones(self) -> Iterator[AlignmentEntry]:
        ...

    def states(self) -> Iterator[AlignmentEntry]:
        ...