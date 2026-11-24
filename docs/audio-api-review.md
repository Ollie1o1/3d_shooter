# Review game audio through the OpenAI API

`tools/review_audio.py` sends actual WAV recordings to `gpt-audio-1.5` and saves
a written critique. It uses Python's standard library: no package installation
or game rebuild is required. This adds an audio-capable review model to the
workflow; it does not give the coding agent direct hearing or guarantee that
the model's aesthetic judgment matches a player.

The implementation follows OpenAI's [audio-input guide](https://developers.openai.com/api/docs/guides/audio-chat-completions)
and [audio model specification](https://developers.openai.com/api/docs/models/gpt-audio-1.5).
It uses Chat Completions, actual `input_audio` content, text-only output and
`store: false`. The game never loads the API credential or calls this API.

## What you need to do

1. Enable billing or available credits on your [OpenAI API account](https://platform.openai.com/account/billing/overview).
   API usage has [separate billing from ChatGPT](https://help.openai.com/en/articles/9039756-managing-billing-for-chatgpt-and-the-api-platform).
2. Create an [API key](https://platform.openai.com/api-keys) in a project with
   access to `gpt-audio-1.5`.
3. In your own terminal, from this project directory, run:

   ```sh
   python3 tools/review_audio.py --configure
   python3 tools/review_audio.py --check
   ```

Paste the key into the hidden terminal prompt. The command saves it to
`config.local.audio.json`, which is ignored by Git. On macOS/Linux the file is
created with owner-only permissions (`0600`). The key is not displayed. Future
agent commands in this project can use that file without needing a new shell
environment. Do not put the key in a chat message or commit it.

If `OPENAI_API_KEY` is already set, it takes precedence over the file. An expired
environment key can therefore override a newly configured local key. `--check`
checks authentication and model visibility without uploading audio. A restricted
key needs permission to read that model as well as create Chat Completions.
This check does not prove that a paid audio request has succeeded.

## First review

Inspect the exact selected recordings without credentials, uploads or charges:

```sh
make audio-review-plan
```

The existing Chapter 6 files produce six clips, totaling 106.13 seconds:

- `build/shepherd-audio/attack-language.wav`: isolated effects and quiet gaps.
- `build/shepherd-audio/live-playthrough.wav`: actual scripted gameplay,
  including movement, shots, impacts, music changes and the ending.

The plan is saved to `build/audio-review/plan.json`. Audio is split into at most
20-second PCM WAV clips with a 4 MiB PCM-data cap per request. Samples, levels,
channel count and sample rate are preserved; no normalization or stereo folding
is applied. Boundary splits can interrupt an event, so critiques explicitly use
clip-local approximate timecodes and can flag missing context.

Once the credential check passes, start with one audio request:

```sh
python3 tools/review_audio.py --limit 1
```

This is a real, billable audio-input review. The full review reuses the first
response and requests the remaining clips:

```sh
make audio-review
```

Read `build/audio-review/review.md`. Individual response records in
`build/audio-review/responses/` include the source path, excerpt bounds, audio
SHA-256, requested/returned model, response/request IDs, usage and critique.
They contain neither API authorization headers nor embedded audio data.
The model first describes the audible recording, then identifies problems,
specific edits and uncertainty. The prompt does not tell it which attack
signature it is supposed to hear.

A coding agent can now read that audio-based feedback and make a small revision,
render it, and request another review. The player remains the final judge.

## Individual sounds and alternatives

Pass your own 16-bit mono/stereo PCM WAVs:

```sh
python3 tools/review_audio.py assets/sfx/shep_contact.wav assets/sfx/shep_sweep.wav
python3 tools/review_audio.py build/shepherd-audio/live-playthrough.wav --context 'Assess whether impacts are masked by walking or music.'
```

Use a separate output folder to keep separate audition rounds:

```sh
python3 tools/review_audio.py --output build/audio-review/round-2
```

The CLI accepts `--model` (or `OPENAI_AUDIO_MODEL`), `--clip-seconds`,
`--max-output-tokens`, and `--limit`. The default is written feedback only, with
at most 1,600 output tokens per clip. An incomplete response is saved and marked;
increase the token limit for a replacement rather than treating it as finished.

## Charges, caching and failures

See the model's [current pricing](https://developers.openai.com/api/docs/models/gpt-audio-1.5).
The script records reported token usage and makes no fixed dollar estimate.
Exact matching audio, model, prompts and token limit reuse cached responses.
Changed audio or settings produce a fresh paid request; `--refresh` explicitly
requests fresh reviews even when a cache exists. Moving to an empty output
directory also creates a new cache and can incur new charges.

Completed clips are saved after each response. There are no automatic retries,
including after a timeout, since an uncertain failed connection can still have
been billed. A later rate-limit/quota failure preserves earlier reviews.
Authentication, model-access and quota errors produce short actionable messages.
Keep the cache when rerunning to avoid repeating successful requests.

## Verification status

`make test-audio-review` exercises hidden credential entry/storage, real WAV
bytes in the request, lossless stereo splitting and remainders, missing keys,
dry-run isolation, secret-free output, HTTP errors, partial results and cache
invalidation using a mocked API transport. A dry run also passed against the
actual Chapter 6 recordings.

No real audio-model request has been made during setup: the API key still needs
to be configured. Live authentication, model access and audio acceptance must be
verified after that step; a mocked test is not a listening review.
