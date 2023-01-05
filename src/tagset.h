#ifndef TAGSET_H
#define TAGSET_H

GESET_DEF_API(char *, tagset)

void tagset_deep_free(tagset_t *);
void parsetags(tagset_t *, char *);

#endif /* !TAGSET_H */
