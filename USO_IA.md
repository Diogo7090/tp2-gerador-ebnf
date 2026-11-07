## USO_IA.md

Registre aqui cada uso relevante de ferramentas de IA generativa, como no TP1. Se nenhuma foi usada, deixe apenas a frase:

Nenhuma ferramenta de IA foi utilizada no desenvolvimento deste trabalho.

Modelo de entrada:

    Ferramenta: <ferramenta e modelo>
    Trecho:     <arquivo ou parte do trabalho>
    Finalidade: <o que foi pedido, de forma especifica>
    O que fiz:  <usado integralmente, adaptado ou apenas consultado>

Ferramenta: ChatGPT (GPT-6)
Trecho: microc/microc.flex — opções do Flex e integração do scanner
Finalidade: Entender onde declarar o prefixo do scanner, a opção noyywrap e como essas configurações afetam a interface gerada e sua integração.
O que fiz: Consultei a análise do arquivo para orientar minha própria adaptação e verificação.

Ferramenta: ChatGPT (GPT-6)

Trecho: src/verificacao.c — verif_inalcancaveis() e verif_improdutivos()

Finalidade: Compreender os algoritmos de busca em profundidade (DFS) para identificar não terminais inalcançáveis e de ponto fixo para identificar não terminais improdutivos, incluindo a avaliação recursiva dos tipos de expressão da árvore Expr e o tratamento das dependências entre não terminais.

O que fiz: Consultei explicações conceituais, análises do raciocínio de implementação e sugestões de organização das funções auxiliares. Utilizei as orientações para desenvolver e adaptar minha própria implementação.

Trecho: src/verificacao.c (implementação das funções verif_inalcancaveis e verif_improdutivos, auxiliares recursivas) e gerenciamento de versionamento (Git)
Finalidade: Auxiliar na concepção e validação dos algoritmos de verificação semântica — especificamente busca em profundidade (DFS) para inalcançáveis e ponto fixo para improdutivos —, além de orientar a resolução de conflitos de merge e o fluxo de sincronização entre as branches local e remota.
O que fiz: Consultei explicações conceituais, orientações de estruturação de código e diretrizes para o fluxo de versionamento, adaptando e aplicando os conceitos no desenvolvimento e organização da etapa.

Ferramenta: Gemini (Google)

Trecho: src/verificacao.c — verif_recursao_esquerda() e recursaoAEsq()

Finalidade: Auxiliar na concepção do algoritmo de detecção de recursão à esquerda (direta e indireta) em árvores de expressão, contemplando o avanço em sequências (`E_SEQ`) com suporte a prefixos anuláveis (`anulavel`), controle de histórico para evitar loops em ciclos e prevenção de falhas de segmentação (*Segmentation Faults*) com checagens de ponteiros nulos e limites de vetores.

O que fiz: Consultei sugestões de estruturação de código, análises de fluxo para tratamento de nós anuláveis e diretrizes de blindagem contra erros de memória, adaptando e aplicando os ajustes diretamente na minha implementação.

Ferramenta: Gemini (Google)

Trecho: src/ll1.c — ll1_verificar() e verificar_expr()

Finalidade: Auxiliar na modelagem da travessia recursiva por descida (verificar_expr) para validação de conflitos LL(1) em árvores de expressão EBNF, mapeando condições de interseção entre conjuntos FIRST e FOLLOW para construções opcionais e de repetição (E_OPC/E_REP), além de estruturar o preenchimento e a verificação de colisões na tabela preditiva.

O que fiz: Consultei orientações lógicas para o mapeamento e validação cruzada de bitsets, estratégias de travessia recursiva em nós compostos e critérios de conformidade de mensagens de diagnóstico com a suite de testes, adaptando e aplicando os conceitos diretamente na implementação do código-fonte.