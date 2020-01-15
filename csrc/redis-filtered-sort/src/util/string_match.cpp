#include "string_match.h"

static const char *match(MatchState *ms, const char *s, const char *p);

MatchRange_t *new_match_range(int start, int end)
{
  MatchRange_t *mr = (MatchRange_t *)malloc(sizeof(MatchRange_t));
  mr->start = start;
  mr->end = end;
  return mr;
}

MatchString_t *new_match_string(const char *str, int len)
{
  MatchString_t *ms = (MatchString_t *)malloc(sizeof(MatchString_t));
  ms->value = strdup(str);
  ms->size = len;
  return ms;
}

Match_t *new_match(MatchType_t t, void *match)
{
  Match_t *mt = (Match_t *)malloc(sizeof(Match_t));
  mt->type = t;
  mt->value = match;
  return mt;
}

int add_match(MatchResult_t *mr, Match_t *m)
{
  mr->size++;
  mr->data = (Match_t **)realloc(mr->data, sizeof(Match_t *) * mr->size);
  mr->data[mr->size - 1] = m;
  return 0;
}

MatchResult_t *new_match_result()
{
  MatchResult_t *mr = (MatchResult_t *)malloc(sizeof(MatchResult_t));
  mr->size = 0;
  mr->data = (Match_t **)malloc(sizeof(Match_t *));
  return mr;
}

int free_match(Match_t *m)
{
  if (m->type == MATCH_STRING)
  {
    MatchString_t *msv = match_string_value(m);
    free((void *)msv->value);
  }
  free(m->value);
  free(m);
  m = NULL;
  return 0;
}

int free_match_result(MatchResult_t *mr)
{

  for (size_t i = 0; i < mr->size; i++)
  {
    free_match(mr->data[i]);
  }

  for (size_t i = mr->error_count; i > 0; i--)
  {
    free((void *)mr->error[i - 1]);
  }

  free(mr->data);
  free(mr->error);
  free(mr);

  mr = NULL;
  return 0;
}

int free_match_state(MatchState *ms)
{
  free(ms);
  return 0;
}

MatchRange_t *match_range_value(Match_t *m)
{
  MatchRange_t *mr = (MatchRange_t *)m->value;
  return mr;
}

MatchString_t *match_string_value(Match_t *m)
{
  MatchString_t *ms = (MatchString_t *)m->value;
  return ms;
}

static int seterror(MatchState *ms, const char *fmt, ...)
{
  ms->error_count++;
  if (ms->error == NULL)
  {
    ms->error = (const char **)malloc(sizeof(char *));
  }
  else
  {
    ms->error = (const char **)realloc(ms->error, sizeof(char *) * ms->error_count);
  }

  va_list argp;
  va_start(argp, fmt);

  ms->error[ms->error_count - 1] = (const char *)malloc(sizeof(char) * 512);
  vsprintf((char *)ms->error[ms->error_count - 1], fmt, argp);

  va_end(argp);

  return 1;
}

static int reprepstate(MatchState *ms)
{
  ms->level = 0;
  //Was assert check. But we don't want process to panic
  if (ms->matchdepth != MAXCCALLS)
    return -1;
  return 0;
}

static void prepstate(MatchState *ms, const char *s, size_t ls, const char *p, size_t lp)
{
  ms->matchdepth = MAXCCALLS;
  ms->src_init = s;
  ms->src_end = s + ls;
  ms->p_end = p + lp;
  ms->error = NULL;
  ms->error_count = 0;
  ms->result = new_match_result();
}

static const char *classend(MatchState *ms, const char *p)
{
  switch (*p++)
  {
  case ESC_CHAR:
  {
    if (p == ms->p_end)
      seterror(ms, "malformed pattern (ends with '%%')");
    return p + 1;
  }
  case '[':
  {
    if (*p == '^')
      p++;
    do
    { // look for a ']'
      if (p == ms->p_end)
      {
        seterror(ms, "malformed pattern (missing ']')");
        return NULL;
      }

      if (*(p++) == ESC_CHAR && p < ms->p_end)
        p++; // skip escapes (e.g. '%]')
    } while (*p != ']');
    return p + 1;
  }
  default:
  {
    return p;
  }
  }
}

static int match_class(int c, int cl)
{
  int res;
  switch (tolower(cl))
  {
  case 'a':
    res = isalpha(c);
    break;
  case 'c':
    res = iscntrl(c);
    break;
  case 'd':
    res = isdigit(c);
    break;
  case 'g':
    res = isgraph(c);
    break;
  case 'l':
    res = islower(c);
    break;
  case 'p':
    res = ispunct(c);
    break;
  case 's':
    res = isspace(c);
    break;
  case 'u':
    res = isupper(c);
    break;
  case 'w':
    res = isalnum(c);
    break;
  case 'x':
    res = isxdigit(c);
    break;
  case 'z':
    res = (c == 0);
    break; // deprecated option
  default:
    return (cl == c);
  }
  return (islower(cl) ? res : !res);
}

static int matchbracketclass(int c, const char *p, const char *ec)
{
  int sig = 1;
  if (*(p + 1) == '^')
  {
    sig = 0;
    p++; // skip the '^'
  }
  while (++p < ec)
  {
    if (*p == ESC_CHAR)
    {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p + 1) == '-') && (p + 2 < ec))
    {
      p += 2;
      if (uchar(*(p - 2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c)
      return sig;
  }
  return !sig;
}

static int singlematch(MatchState *ms, const char *s, const char *p, const char *ep)
{
  if (s >= ms->src_end)
    return 0;
  else
  {
    int c = uchar(*s);
    switch (*p)
    {
    case '.':
      return 1; // matches any char
    case ESC_CHAR:
      return match_class(c, uchar(*(p + 1)));
    case '[':
      return matchbracketclass(c, p, ep - 1);
    default:
      return (uchar(*p) == c);
    }
  }
}

static const char *max_expand(MatchState *ms, const char *s, const char *p, const char *ep)
{
  ptrdiff_t i = 0; // counts maximum expand for item
  while (singlematch(ms, s + i, p, ep))
    i++;
  //keeps trying to match with the maximum repetitions
  while (i >= 0)
  {
    const char *res = match(ms, (s + i), ep + 1);
    if (res)
      return res;
    i--; // else didn't match; reduce 1 repetition to try again
  }
  return NULL;
}

static const char *min_expand(MatchState *ms, const char *s, const char *p, const char *ep)
{
  for (;;)
  {
    const char *res = match(ms, s, ep + 1);
    if (res != NULL)
      return res;
    else if (singlematch(ms, s, p, ep))
      s++; // try with one more repetition
    else
      return NULL;
  }
}

static const char *matchbalance(MatchState *ms, const char *s, const char *p)
{
  if (p >= ms->p_end - 1)
    seterror(ms, "malformed pattern (missing arguments to '%%b')");
  if (*s != *p)
    return NULL;
  else
  {
    int b = *p;
    int e = *(p + 1);
    int cont = 1;
    while (++s < ms->src_end)
    {
      if (*s == e)
      {
        if (--cont == 0)
          return s + 1;
      }
      else if (*s == b)
        cont++;
    }
  }
  return NULL; // string ends out of balance
}

static int check_capture(MatchState *ms, int l)
{
  l -= '1';
  if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
    return seterror(ms, "invalid capture index %%%d", l + 1);

  return l;
}

static int capture_to_close(MatchState *ms)
{
  int level = ms->level;
  for (level--; level >= 0; level--)
    if (ms->capture[level].len == CAP_UNFINISHED)
      return level;
  return seterror(ms, "invalid pattern capture");
}

static void push_onecapture(MatchState *ms, int i, const char *s, const char *e)
{
  if (i >= ms->level)
  {
    if (i == 0)
    {

      int mstr_size = strlen(s) - strlen(e);
      if (mstr_size > 0)
      {
        char mstrcp[mstr_size + 1];
        strncpy(mstrcp, s, mstr_size);
        mstrcp[mstr_size] = '\0';
        MatchString_t *mstr = new_match_string(mstrcp, mstr_size);

        Match_t *m = new_match(MATCH_STRING, mstr);
        add_match(ms->result, m);
      }
    }
    else
      seterror(ms, "invalid capture index %%%d\n", i + 1);
  }
  else
  {
    ptrdiff_t l = ms->capture[i].len;
    if (l == CAP_UNFINISHED)
    {
      seterror(ms, "unfinished capture");
      return;
    }
    if (l == CAP_POSITION)
    {
      MatchRange_t *mran = new_match_range(ms->capture[i].init - ms->src_init + 1, 0);
      Match_t *m = new_match(MATCH_RANGE, mran);
      add_match(ms->result, m);
    }
    else
    {
      int mstr_size = ms->capture[i].len;
      char mstrcp[mstr_size + 1];
      strncpy(mstrcp, ms->capture[i].init, mstr_size);
      mstrcp[mstr_size] = '\0';

      MatchString_t *mstr = new_match_string(mstrcp, l);
      Match_t *m = new_match(MATCH_STRING, mstr);
      add_match(ms->result, m);
    }
  }
}

static int push_captures(MatchState *ms, const char *s, const char *e)
{
  int i;
  int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
  for (i = 0; i < nlevels; i++)
  {
    push_onecapture(ms, i, s, e);
  }

  return nlevels; // number of strings pushed
}

static const char *start_capture(MatchState *ms, const char *s, const char *p, int what)
{
  const char *res;
  int level = ms->level;
  if (level >= MAXCAPTURES)
    seterror(ms, "too many captures");
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level + 1;
  if ((res = match(ms, s, p)) == NULL) // match failed?
    ms->level--;                       // undo capture
  return res;
}

static const char *end_capture(MatchState *ms, const char *s, const char *p)
{
  int l = capture_to_close(ms);
  const char *res;
  ms->capture[l].len = s - ms->capture[l].init; // close capture
  if ((res = match(ms, s, p)) == NULL)          // match failed?
    ms->capture[l].len = CAP_UNFINISHED;        // undo capture
  return res;
}

static const char *match_capture(MatchState *ms, const char *s, int l)
{
  size_t len;
  l = check_capture(ms, l);
  len = ms->capture[l].len;
  if ((size_t)(ms->src_end - s) >= len && memcmp(ms->capture[l].init, s, len) == 0)
  {
    return s + len;
  }
  else
  {
    return NULL;
  }
}

static const char *match(MatchState *ms, const char *s, const char *p)
{
  if (ms->matchdepth-- == 0)
  {
    seterror(ms, "pattern too complex");
    return s;
  }

init:
  if (p != ms->p_end)
  {
    switch (*p)
    {
    case '(':
    {                      // start capture
      if (*(p + 1) == ')') // position capture?
        s = start_capture(ms, s, p + 2, CAP_POSITION);
      else
        s = start_capture(ms, s, p + 1, CAP_UNFINISHED);
      break;
    }
    case ')':
    { // end capture
      s = end_capture(ms, s, p + 1);
      break;
    }
    case '$':
    {
      if ((p + 1) != ms->p_end)          // is the `$' the last char in pattern?
        goto dflt;                       // no; go to default
      s = (s == ms->src_end) ? s : NULL; // check end of string
      break;
    }
    case ESC_CHAR:
    { // escaped sequences not in the format class[*+?-]?
      switch (*(p + 1))
      {
      case 'b':
      { // balanced string?
        s = matchbalance(ms, s, p + 2);
        if (s != NULL)
        {
          p += 4;
          goto init; // return match(ms, s, p + 4);
        }
        break;
      }
      case 'f':
      { // frontier?
        const char *ep;
        char previous;
        p += 2;
        if (*p != '[')
          seterror(ms, "missing '[' after '%%f' in pattern");

        ep = classend(ms, p); // points to what is next
        if (ep == NULL)
        {
          s = NULL;
          break;
        }
        previous = (s == ms->src_init) ? '\0' : *(s - 1);
        if (!matchbracketclass(uchar(previous), p, ep - 1) &&
            matchbracketclass(uchar(*s), p, ep - 1))
        {
          p = ep;
          goto init; // return match(ms, s, ep);
        }
        s = NULL; // match failed
        break;
      }
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      { // capture results (%0-%9)?
        s = match_capture(ms, s, uchar(*(p + 1)));
        if (s != NULL)
        {
          p += 2;
          goto init; // return match(ms, s, p + 2)
        }
        break;
      }
      default:
        goto dflt;
      }
      break;
    }
    default:
    dflt:
    {                                   // pattern class plus optional suffix
      const char *ep = classend(ms, p); // points to optional suffix
      if (ep == NULL)
      {
        //ms->matchdepth ++;
        return NULL;
      }
      if (!singlematch(ms, s, p, ep))
      {
        if (*ep == '*' || *ep == '?' || *ep == '-')
        { // accept empty?
          p = ep + 1;
          goto init;
        }
        else
          s = NULL;
      }
      else
      { // matched once */
        switch (*ep)
        { // handle optional suffix
        case '?':
        {
          const char *res;
          if ((res = match(ms, s + 1, ep + 1)) != NULL)
            s = res;
          else
          {
            p = ep + 1;
            goto init;
          }
          break;
        }
        case '+':
          s++;

        case '*': // 0 or more
          s = max_expand(ms, s, p, ep);
          break;
        case '-': // 0 or more min
          s = min_expand(ms, s, p, ep);
          break;
        default: // no suffix
          s++;
          p = ep;
          goto init;
        }
      }
      break;
    }
    }
  }
  ms->matchdepth++;
  return s;
}

static int str_match_aux(MatchState *ms, const char *s, const char *p, int find)
{
  size_t ls = strlen(s), lp = strlen(p);
  int init = 1;

  const char *s1 = s + init - 1;
  int anchor = (*p == '^');
  if (anchor)
  {
    p++;
    lp--; // skip "^" anchor character
  }
  prepstate(ms, s, ls, p, lp);
  do
  {
    const char *res;
    if (reprepstate(ms) == -1)
    {
      return 0; //seterror(ms, "too much calls");
    }
    if ((res = match(ms, s1, p)) != NULL)
    {
      if (find)
      {
        int start = (s1 - s) + 1;
        int stop = (s1 - s) + lp;
        MatchRange_t *mran = new_match_range(start, stop);
        Match_t *m = new_match(MATCH_RANGE, mran);
        add_match(ms->result, m);
        int pushres = push_captures(ms, NULL, 0) + 2;
        return pushres;
      }
      else
      {
        int pushres = push_captures(ms, s1, res);
        return pushres;
      }
    }
  } while (s1++ < ms->src_end && !anchor);

  return 0;
}

MatchResult_t *str_match(const char *s, const char *p)
{
  MatchState *ms = (MatchState *)malloc(sizeof(MatchState));
  str_match_aux(ms, s, p, 0);

  MatchResult_t *mr = ms->result;
  mr->error_count = ms->error_count;
  mr->error = ms->error;
  free_match_state(ms);
  return mr;
}

int print_result(MatchResult_t *mr)
{
  printf("MatchCount: %ld\n", mr->size);
  if (mr->error_count > 0)
  {
    for (int i = 0; i < mr->error_count; i++)
    {
      printf("Got %d errors: %s\n", mr->error_count, mr->error[i]);
    }
  }
  for (size_t i = 0; i < mr->size; i++)
  {
    if (mr->data[i]->type == MATCH_STRING)
    {
      MatchString_t *mstr = match_string_value(mr->data[i]);
      printf("...Match type %d value '%s'\n", mstr->size, mstr->value);
    }
    if (mr->data[i]->type == MATCH_RANGE)
    {
      MatchRange_t *mrang = match_range_value(mr->data[i]);
      printf("...Match type range %d value %d %d\n", MATCH_RANGE, mrang->start, mrang->end);
    }
  }
  return 0;
}