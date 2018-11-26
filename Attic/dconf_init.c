// I feel like in many ways this is a better parser
// and that we should instead merge the sections code into here...
#if	0
void dconf_init(const char *file) {
   FILE       *fp;
   char       *p;
   char        buf[512];
   char        *discard = NULL;
   char       *key, *val;
   _DCONF_DICT = dict_new();

   if (!(fp = fopen(file, "r"))) {
      Log(LOG_FATAL, "unable to open config file %s", file);
      raise(SIGTERM);
   }

   while (!feof(fp)) {
      line++;
      memset(buf, 0, 512);
      discard = fgets(buf, sizeof(buf), fp);

      /*
       * did we get a line? 
       */
      if (buf == NULL)
         continue;

      p = buf;

      str_strip(p);
      p = str_whitespace_skip(p);

      /*
       * did we eat the whole line? 
       */
      if (strlen(p) == 0)
         continue;

      /*
       * fairly flexible comment handling
       */
      if (p[0] == '*' && p[1] == '/') {
         in_comment = 0;               /* end of block comment */
         continue;
      } else if (p[0] == ';' || p[0] == '#' || (p[0] == '/' && p[1] == '/')) {
         continue;                     /* line comment */
      } else if (p[0] == '/' && p[1] == '*')
         in_comment = 1;               /* start of block comment */

      if (in_comment)
         continue;                     /* ignored line, in block comment */

      key = p;

      if ((val = strchr(p, '=')) != NULL) {
         *val = '\0';
         val++;
      }

      if (val == NULL)
         continue;

      val = str_unquote(val);
      dconf_set(key, val);
   }
}
#endif	
