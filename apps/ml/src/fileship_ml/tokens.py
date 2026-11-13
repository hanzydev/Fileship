from tokenizers import Tokenizer

CLEANUPS = (
    (' .', '.'),
    (' ?', '?'),
    (' !', '!'),
    (' ,', ','),
    (" ' ", "'"),
    (" n't", "n't"),
    (" 'm", "'m"),
    (" 's", "'s"),
    (" 've", "'ve"),
    (" 're", "'re"),
)


def decode(tokenizer: Tokenizer, ids: list[int], skip_special_tokens: bool = True) -> str:
    text = tokenizer.decode(ids, skip_special_tokens=skip_special_tokens)
    for old, new in CLEANUPS:
        text = text.replace(old, new)

    return text
