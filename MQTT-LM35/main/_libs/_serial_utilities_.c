#ifndef ___SERIAL_UTILITIES_V11_C__

char *str_transfer_from_to_begin_end(char *from, char *to, char what[], char end) {
   uint16_t i, j;
   uint16_t lenght_what, lenght_from;
   bool found = 0;

   lenght_what = strlen(what);
   lenght_from = strlen(from);

   if (lenght_what <= lenght_from) {
      for (i = 0; i <= (lenght_from - lenght_what); i++) {
         found = 0;
         if (lenght_what == 0) found = 1;
         else {
            for (j = 0; j < lenght_what; j++) {
               if (from[i + j] == what[j]) found = 1;
               else
                  found = 0;

               if (!found) break;
            }
         }

         if (found) {
            /* Transefere o que está após o procurado para a resposta.
               Posiciona i logo após acabar what e copia até que from seja end. */
            for (i += lenght_what, j = 0; (from[i] != end && from[i] != '\r' && from[i] != '\n'); i++, j++) {
               to[j] = from[i];
            }

            // Insere um null em to para indicar que acabou
            to[j] = 0;

            return from + i; // Retorna ponteiro se encontrou e transferiu
         }
      }
   }

   return NULL; // Retorna 0 se não encontrou
}

   #define ___SERIAL_UTILITIES_V11_C__
#endif