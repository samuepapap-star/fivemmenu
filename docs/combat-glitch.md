# Combat Glitch da HyperX

O modo está em Personagem → Extras, dentro de Spinbot. Ative Spinbot e Combat Glitch. Mirar + Espaço abre uma janela de giro de 800 ms no chão; desligar, morte, veículo, perda de foco ou flag de ar ilegível cancelam a janela. O botão não chama um native de animação e não afirma confirmar rolamento real.

A implementação reutiliza a escrita de orientação já existente, sem importar a thread da HyperX. A configuração participa da importação/exportação. Testes cobrem expiração, estado no ar, desativação e saturação do prazo.
