/* test f�r lokale variable */

int x=0;

/* der folgende Block enth�lt keine Befehle, aber er sollte x trotzdem
   hochz�hlen. Mal sehen ob das tut ... */
{
    int a=x++;
}

printf ("x = %d (1)\n", x);
