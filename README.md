# BBQ-Nextion

BBQ-Nextion é uma aplicação desenvolvida para o controle preciso e eficiente de um pit smoker. Com esta aplicação, você pode monitorar e ajustar a temperatura do seu pit smoker em tempo real, garantindo sempre o ponto perfeito para suas carnes.

## Funcionalidades

- **Monitoramento de Temperatura em Tempo Real**: Receba dados de temperatura instantaneamente do seu pit smoker, garantindo que você tenha o controle total durante todo o processo de cozimento.
- **Ajuste Preciso da Temperatura**: Ajuste a temperatura do pit smoker diretamente pelo display Nextion, facilitando o controle sem precisar estar fisicamente presente ao lado do equipamento.
- **Alarmes e Notificações**: Configure alarmes para temperaturas específicas, garantindo que você seja avisado quando a temperatura estiver fora do intervalo desejado.
- **Histórico de Temperatura**: Visualize o histórico de temperaturas, permitindo uma análise detalhada do desempenho do seu pit smoker ao longo do tempo.
- **Interface Intuitiva**: Interface de usuário amigável e fácil de usar, desenvolvida com o display Nextion, proporcionando uma experiência de usuário fluida e eficiente.

## Requisitos de Hardware

- **ESP32-S3 DevKitC-1 N16R8**: Placa de desenvolvimento ESP32-S3.
- **Display Nextion**: Display touchscreen para interface com o usuário.
- **Sensores de Temperatura ACS712**: Sensores para monitoramento da temperatura do pit smoker.
- **Conversor TTL/RS232**: Para comunicação entre o display Nextion e o ESP32.

## Configuração

1. **Conexão de Hardware**:
    - Conecte os sensores ACS712 ao ESP32-S3.
    - Conecte o display Nextion ao ESP32-S3 usando o conversor TTL/RS232.
    - Garanta que todas as conexões estejam firmes e corretas.

2. **Instalação do Software**:
    - Clone o repositório BBQ-Nextion do GitHub.
    - Use o PlatformIO para configurar e compilar o código.
    - Carregue o código compilado na placa ESP32-S3.

3. **Configuração do Display Nextion**:
    - Carregue a interface desenvolvida no display Nextion.
    - Configure os parâmetros iniciais conforme necessário para o seu pit smoker.

## Uso

- **Inicialização**: Ligue o seu pit smoker e a aplicação BBQ-Nextion. O display Nextion exibirá as temperaturas atuais dos sensores.
- **Ajuste de Temperatura**: Use a interface do display para ajustar a temperatura desejada do pit smoker.
- **Monitoramento**: Monitore a temperatura em tempo real e receba alertas caso a temperatura saia do intervalo configurado.

## Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues e pull requests no repositório GitHub.

## Licença

Este projeto está licenciado sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.

---

Com BBQ-Nextion, seu pit smoker estará sempre sob controle, garantindo carnes perfeitamente cozidas todas as vezes. Aproveite e bom churrasco!
