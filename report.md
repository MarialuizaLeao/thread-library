# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua marcadores `XX` pela contribuição de cada membro
do grupo no desenvolvimento do trabalho (os valores devem somar
100%).

  * Maria Luiza Leao <maria_luiza1598@hotmail.com> 50%
  * Isabella Vignoli Gonçalves <isabellavignoli14@gmail.com> 50%

3. Referências bibliográficas:
    https://petbcc.ufscar.br/time/
    https://www.tutorialspoint.com/c_standard_library/time_h.htm
    https://www.ibm.com/docs/en/i/7.2?topic=ssw_ibm_i_72/apis/sigactn.html
    https://man7.org/linux/man-pages/man2/sigaction.2.html
    https://virtual.ufmg.br/20231/pluginfile.php/286104/mod_resource/content/0/06-Sincronizacao.pdf
    https://www.geeksforgeeks.org/use-posix-semaphores-c/
    https://greenteapress.com/thinkos/html/thinkos013.html
    https://man7.org/linux/man-pages/man3/makecontext.3.html
    https://man7.org/linux/man-pages/man3/getcontext.3.html
    https://man7.org/linux/man-pages/man3/swapcontext.3.html
    https://man7.org/linux/man-pages/man3/setcontext.3.html


4. Estruturas de dados

  1. Descreva e justifique as estruturas de dados utilizadas para
    As estruturas de dados utilizadas na implementação da biblioteca de threads de espaço do usuário apresentada são:

dccthread_t: estrutura de dados que representa uma thread. Possui os seguintes campos:
name: nome da thread, uma string com tamanho máximo de DCCTHREAD_MAX_NAME_SIZE.
context: ponteiro para o contexto da thread.
isWaiting: booleano que indica se a thread está esperando (true) ou não (false).
waitingFor: ponteiro para a thread pela qual a thread atual está esperando.
bin_sem_t: estrutura de dados que representa um semáforo binário. Possui os seguintes campos:
flag: valor do semáforo, que pode ser 0 (semáforo vermelho) ou 1 (semáforo verde).
guard: variável de guarda, utilizada para evitar race conditions no acesso ao semáforo.
sleepingThreadList: lista encadeada que contém as threads que estão esperando pelo semáforo.
Além disso, a biblioteca também utiliza a seguinte estrutura de dados:

dlist: lista encadeada duplamente ligada que contém as threads prontas para serem executadas.
Justificativas:

dccthread_t: é uma estrutura de dados que armazena as informações de cada thread, tais como nome, contexto e estado. Essas informações são necessárias para a criação, execução, espera e término das threads.
bin_sem_t: é uma estrutura de dados que representa um semáforo binário, utilizado para controlar o acesso a recursos compartilhados pelas threads. É necessário ter uma lista de threads que estão esperando pelo semáforo para que seja possível acordá-las quando o semáforo for liberado.
dlist: é uma estrutura de dados que permite adicionar e remover elementos em tempo constante no início e no final da lista. Essa estrutura é ideal para armazenar as threads prontas para serem executadas, pois a cada mudança de contexto é necessário escolher a próxima thread a ser executada, que é sempre a primeira da lista.

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).
     
O mecanismo utilizado para sincronizar chamadas de dccthread_yield e disparos do temporizador pode variar dependendo da implementação específica do sistema ou biblioteca que está sendo usada. No entanto, em geral, a sincronização pode ser realizada usando semáforos, mutexes ou outros mecanismos de exclusão mútua para garantir que apenas um thread possa executar de cada vez.

Por exemplo, quando um temporizador é disparado, ele pode sinalizar um semáforo que está sendo esperado pelo thread que está atualmente em execução. Quando o thread recebe o sinal, ele pode então liberar o semáforo e entrar em uma seção crítica protegida por um mutex. Dentro dessa seção crítica, ele pode executar uma verificação para determinar se ele deve chamar dccthread_yield e, se assim for, chamar a função.

No entanto, existem muitas abordagens diferentes para sincronizar threads e temporizadores, e a melhor abordagem dependerá dos detalhes específicos do sistema ou biblioteca em questão. É importante garantir que a sincronização seja feita de forma segura e eficiente para evitar problemas de concorrência e desempenho.