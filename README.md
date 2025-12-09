# Ray Tracer - Seminário PPD

Implementação de um Ray Tracer com múltiplas versões paralelas para comparação de desempenho.

## 📁 Estrutura do Projeto

```
seminario-ppd/
├── imagem/                  # Ray tracer para geração de imagens estáticas
│   ├── raytracer.cpp        # Versão sequencial
│   ├── raytracer_omp.cpp    # Versão paralela com OpenMP
│   └── input.in             # Arquivo de entrada
│
├── interativo/              # Ray tracer interativo em tempo real
│   ├── raytracer_interativo.c    # Versão OpenMP (CPU)
│   ├── raytracer_interativo.cu   # Versão CUDA (GPU)
│   ├── comparar_fps.py           # Dashboard de comparação
│   ├── fps_omp.txt               # Dados de FPS do OpenMP (gerado ao executar)
│   └── fps_cuda.txt              # Dados de FPS do CUDA (gerado ao executar)
│
├── Makefile
└── README.md
```

## 🔧 Dependências

### Para compilação:
- **GCC** (versão com suporte a C11)
- **G++** (versão 10 ou superior)
- **NVCC** (NVIDIA CUDA Compiler)
- **OpenMP** (geralmente incluído no GCC)
- **SDL2** (para visualização interativa)

### Para análise de dados:
- **Python 3.8+**
- **Streamlit**
- **Pandas**
- **Plotly**

### Instalação das dependências no Ubuntu/Debian:

```bash
# Compiladores e bibliotecas
sudo apt update
sudo apt install build-essential g++-10 libsdl2-dev

# CUDA (siga as instruções oficiais da NVIDIA)
# https://developer.nvidia.com/cuda-downloads

# Python e bibliotecas para análise
pip install streamlit pandas plotly
```

## 🛠️ Compilação

### 1. Ray Tracer de Imagens (pasta `imagem/`)

```bash
cd imagem/

# Versão sequencial
g++ -O3 raytracer.cpp -o raytracer

# Versão OpenMP
g++ -O3 raytracer_omp.cpp -o raytracer_omp -fopenmp

# Ou usar o Makefile:
make omp
```

### 2. Ray Tracer Interativo (pasta `interativo/`)

```bash
cd interativo/

# Versão OpenMP (CPU)
gcc -O3 -o rayview_omp raytracer_interativo.c -lm -fopenmp `sdl2-config --cflags --libs`

# Versão CUDA (GPU)
nvcc -O3 -Xcompiler -fopenmp -ccbin g++-10 -o rayview_cuda raytracer_interativo.cu -lm `sdl2-config --cflags --libs`
```

## 🚀 Execução

### Ray Tracer de Imagens

```bash
cd imagem/

# Versão sequencial
./raytracer < input.in

# Versão OpenMP
./raytracer_omp < input.in
```

Ambos os programas geram arquivos de saída com as imagens renderizadas.

### Ray Tracer Interativo

#### Versão OpenMP (CPU):
```bash
cd interativo/
./rayview_omp
```

#### Versão CUDA (GPU):
```bash
cd interativo/
./rayview_cuda
```

#### Controles:
- **W/S**: Mover para frente/trás
- **A/D**: Mover para esquerda/direita
- **Q/E**: Mover para cima/baixo
- **Setas**: Rotacionar câmera
- **Mouse**: Clique para capturar/liberar mouse (movimentação da câmera)
- **ESC**: Sair

Ao executar os programas, arquivos `fps_omp.txt` e `fps_cuda.txt` são gerados automaticamente com os dados de desempenho.

## 📊 Análise de Desempenho com Streamlit

Após executar os programas interativos e gerar os arquivos de FPS, você pode visualizar uma comparação detalhada:

```bash
cd interativo/
streamlit run comparar_fps.py
```

O dashboard será aberto automaticamente no navegador em `http://localhost:8501` (ou 8502).

### O que o dashboard mostra:

1. **Métricas Principais**:
   - FPS médio de cada implementação (OpenMP vs CUDA)
   - Speedup (ganho de performance da GPU sobre CPU)

2. **Gráficos de Linha**:
   - Comparação de FPS ao longo do tempo
   - Análise de estabilidade de cada implementação

3. **Histogramas**:
   - Distribuição de FPS de cada versão
   - Identificação de padrões de performance

### Workflow de análise:

```bash
# 1. Execute a versão OpenMP por alguns segundos
./rayview_omp
# (Feche com ESC após coletar dados)

# 2. Execute a versão CUDA por alguns segundos
./rayview_cuda
# (Feche com ESC após coletar dados)

# 3. Compare os resultados
streamlit run comparar_fps.py
```

## 📈 Formato dos Dados

Os arquivos `fps_omp.txt` e `fps_cuda.txt` são gerados em formato CSV:

```csv
Tempo(s),FPS
1.00,45.23
2.00,48.56
3.00,47.89
...
```

O dashboard automaticamente:
- Iguala o número de registros entre os dois arquivos
- Calcula estatísticas comparativas
- Gera visualizações interativas

## 🎯 Características Técnicas

### Ray Tracer de Imagens:
- Renderização offline de alta qualidade
- Reflexões e refrações
- Múltiplas esferas e fontes de luz
- Otimização com OpenMP para CPU multi-core

### Ray Tracer Interativo:
- Renderização em tempo real (1280x720)
- Câmera livre com controles de movimento
- Paralelização massiva com CUDA (GPU)
- Paralelização com OpenMP (CPU)
- Medição automática de FPS

### Otimizações CUDA:
- Uso de memória constante para objetos da cena
- Configuração otimizada de blocos e threads (16x16)
- Sincronização eficiente entre CPU e GPU
- Reutilização de memória alocada entre frames

## 📝 Notas

- Os arquivos de FPS são sobrescritos a cada execução
- É recomendado executar cada versão por pelo menos 10-15 segundos para obter dados significativos
- O CUDA requer uma GPU NVIDIA com drivers apropriados instalados
- Para melhor comparação, execute ambos os programas na mesma cena e condições

## 🐛 Troubleshooting

### Erro de compilação CUDA:
```bash
# Certifique-se de usar g++-10 como host compiler
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10
```

### SDL2 não encontrado:
```bash
sudo apt install libsdl2-dev
```

### Streamlit não inicia:
```bash
# Verifique se está no diretório correto
cd interativo/
streamlit run comparar_fps.py
```

### Dashboard não mostra dados:
```bash
# Verifique se os arquivos de FPS existem
ls -l fps_*.txt

# Se não existirem, execute os programas primeiro
./rayview_omp   # Execute e depois feche com ESC
./rayview_cuda  # Execute e depois feche com ESC
```

## 👥 Autores

Projeto desenvolvido para o Seminário de PPD (Processamento Paralelo e Distribuído).

## 📄 Licença

Este projeto é de código aberto para fins educacionais.

