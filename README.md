# Chess-engine-C

A chess engine written from scratch in C (~1,050 hand-written lines), as a personal project to get good at low-level, performance-oriented programming.

![Architecture](architecture.png)

**Work in progress — there is no playable engine yet.** `main` is currently an empty debug driver, so the binary does nothing.

### Done
- **Board:** bitboards (one per colour per piece type) plus a mailbox array; FEN parsing into a `Position`.
- **Attacks:** precomputed tables for knight, king and pawns; **magic bitboards** for sliders, with magics searched at startup.
- **Move generation:** pseudo-legal moves for all pieces, including castling, en passant and promotions, in a 16-bit move encoding.
- **Verified:** differentially tested against python-chess over **104,720 positions — 0 mismatches**.

### Next
make / unmake move → legality check → perft → later: search (alpha-beta), evaluation, UCI.

### Build
Requires gcc and make (Linux):

```
make
```

Output goes to `build/bin/engine`. Sources in `src/`, headers in `include/`.
