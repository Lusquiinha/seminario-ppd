import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import plotly.express as px
from pathlib import Path

st.set_page_config(page_title="Comparação Ray Tracer - PPD", layout="wide")

st.title("📊 Comparação de Desempenho: Ray Tracer")
st.markdown("### Análise Completa - Geração de Imagem e Interativo")

# Define os diretórios corretos
script_dir = Path(__file__).parent

# Todos os arquivos estão no diretório benchmark
fps_seq_path = script_dir / 'fps_seq.txt'
fps_omp_path = script_dir / 'fps_omp.txt'
fps_omp_static_path = script_dir / 'fps_omp_static.txt'
fps_cuda_path = script_dir / 'fps_cuda.txt'
time_raytracer_path = script_dir / 'time_raytracer.txt'

st.markdown("---")

# ============ SEÇÃO 1: GERAÇÃO DE IMAGEM ============
st.header("Geração de Imagem")

if time_raytracer_path.exists():
    try:
        df_time = pd.read_csv(time_raytracer_path)
        
        col1, col2, col3 = st.columns(3)
        
        time_seq = df_time[df_time['tipo'] == 'seq']['time(s)'].values[0] if 'seq' in df_time['tipo'].values else None
        time_omp = df_time[df_time['tipo'] == 'omp']['time(s)'].values[0] if 'omp' in df_time['tipo'].values else None
        
        with col1:
            if time_seq:
                st.metric("Tempo - Sequencial", f"{time_seq:.3f}s")
        
        with col2:
            if time_omp:
                st.metric("Tempo - OpenMP", f"{time_omp:.3f}s")
        
        with col3:
            if time_seq and time_omp:
                speedup = time_seq / time_omp
                st.metric("Speedup OpenMP/Seq", f"{speedup:.2f}x")
        
        # Gráfico de barras
        fig_bar = go.Figure()
        
        if time_seq:
            fig_bar.add_trace(go.Bar(
                name='Sequencial',
                x=['Tempo de Execução'],
                y=[time_seq],
                marker_color='#d62728',
                text=[f"{time_seq:.3f}s"],
                textposition='auto'
            ))
        
        if time_omp:
            fig_bar.add_trace(go.Bar(
                name='OpenMP',
                x=['Tempo de Execução'],
                y=[time_omp],
                marker_color='#1f77b4',
                text=[f"{time_omp:.3f}s"],
                textposition='auto'
            ))
        
        fig_bar.update_layout(
            title="Tempo de Execução - Geração de Imagem",
            yaxis_title="Tempo (segundos)",
            height=400,
            barmode='group'
        )
        
        st.plotly_chart(fig_bar, use_container_width=True)
        
    except Exception as e:
        st.warning(f"⚠️ Erro ao ler dados de imagem: {e}")
else:
    st.info("""
    📌 Adicione o arquivo `time_raytracer.txt` neste diretório (`benchmark/`) com o formato:
    
    ```csv
    time(s),tipo
    2.043,seq
    0.729,omp
    ```
    
    Execute os programas de geração de imagem no diretório `imagem/` e meça os tempos.
    """)

st.markdown("---")

# ============ AY TRACER INTERATIVO ============
st.header("Ray Tracer Interativo")

# Tenta ler os arquivos
try:
    # Lê arquivos disponíveis
    dfs = {}
    names = []
    
    if fps_seq_path.exists():
        dfs['Sequencial'] = pd.read_csv(fps_seq_path)
        names.append('Sequencial')
    
    if fps_omp_static_path.exists():
        dfs['OpenMP Static'] = pd.read_csv(fps_omp_static_path)
        names.append('OpenMP Static')
        
    if fps_omp_path.exists():
        dfs['OpenMP Dynamic'] = pd.read_csv(fps_omp_path)
        names.append('OpenMP Dynamic')
    
    if fps_cuda_path.exists():
        dfs['CUDA'] = pd.read_csv(fps_cuda_path)
        names.append('CUDA')
    
    if not dfs:
        st.error("❌ Nenhum arquivo de FPS encontrado. Execute os programas e copie os arquivos para `benchmark/`!")
        st.info("""
        **Os arquivos esperados no diretório `benchmark/`:**
        - `fps_seq.txt` - Sequencial
        - `fps_omp.txt` - OpenMP Dynamic
        - `fps_omp_static.txt` - OpenMP Static  
        - `fps_cuda.txt` - CUDA
        """)
        st.stop()
    
    # Iguala o número de registros (pega o mínimo)
    min_records = min(len(df) for df in dfs.values())
    for name in dfs:
        dfs[name] = dfs[name].iloc[:min_records]
    
    # Estatísticas gerais
    num_cols = len(names) + (1 if 'Sequencial' in dfs and len(names) > 1 else 0)
    cols = st.columns(num_cols)
    
    for idx, name in enumerate(names):
        with cols[idx]:
            fps_mean = dfs[name]['FPS'].mean()
            st.metric(
                f"FPS Médio - {name}",
                f"{fps_mean:.2f}"
            )
    
    # Speedup relativo ao sequencial (se existir)
    if 'Sequencial' in dfs and len(names) > 1:
        with cols[-1]:
            base_fps = dfs['Sequencial']['FPS'].mean()
            if 'CUDA' in dfs:
                speedup = dfs['CUDA']['FPS'].mean() / base_fps
                st.metric(
                    "Speedup CUDA/Seq",
                    f"{speedup:.2f}x"
                )
            elif 'OpenMP Static' in dfs:
                speedup = dfs['OpenMP Static']['FPS'].mean() / base_fps
                st.metric(
                    "Speedup OMP/Seq",
                    f"{speedup:.2f}x"
                )
            elif 'OpenMP Dynamic' in dfs:
                speedup = dfs['OpenMP Dynamic']['FPS'].mean() / base_fps
                st.metric(
                    "Speedup OMP/Seq",
                    f"{speedup:.2f}x"
                )
    
    # Gráfico de linha comparativo
    st.subheader("📈 FPS ao Longo do Tempo")
    
    fig = go.Figure()
    
    colors = {
        'Sequencial': '#d62728', 
        'OpenMP Dynamic': '#1f77b4', 
        'OpenMP Static': '#2ca02c',
        'CUDA': '#ff7f0e'
    }
    
    for name in names:
        fig.add_trace(go.Scatter(
            x=dfs[name]['Tempo(s)'],
            y=dfs[name]['FPS'],
            mode='lines+markers',
            name=name,
            line=dict(color=colors[name], width=2),
            marker=dict(size=6)
        ))
    
    fig.update_layout(
        xaxis_title="Tempo (segundos)",
        yaxis_title="FPS",
        hovermode='x unified',
        height=500,
        legend=dict(
            yanchor="top",
            y=0.99,
            xanchor="left",
            x=0.01
        )
    )
    
    st.plotly_chart(fig, use_container_width=True)
    
    # Gráficos de distribuição
    st.subheader("📊 Distribuição de FPS")
    
    cols_dist = st.columns(len(names))
    
    for idx, name in enumerate(names):
        with cols_dist[idx]:
            st.markdown(f"**{name}**")
            fig_hist = px.histogram(
                dfs[name],
                x='FPS',
                nbins=30,
                color_discrete_sequence=[colors[name]]
            )
            fig_hist.update_layout(
                xaxis_title="FPS",
                yaxis_title="Frequência",
                showlegend=False,
                height=350
            )
            st.plotly_chart(fig_hist, use_container_width=True)
    
    # Tabela comparativa
    st.subheader("📋 Resumo Estatístico")
    
    stats_data = []
    for name in names:
        stats_data.append({
            'Implementação': name,
            'FPS Médio': f"{dfs[name]['FPS'].mean():.2f}",
            'FPS Mín': f"{dfs[name]['FPS'].min():.2f}",
            'FPS Máx': f"{dfs[name]['FPS'].max():.2f}",
            'Desvio Padrão': f"{dfs[name]['FPS'].std():.2f}",
        })
    
    if 'Sequencial' in dfs:
        base_fps = dfs['Sequencial']['FPS'].mean()
        for row in stats_data:
            impl = row['Implementação']
            if impl != 'Sequencial':
                speedup = dfs[impl]['FPS'].mean() / base_fps
                row['Speedup vs Seq'] = f"{speedup:.2f}x"
            else:
                row['Speedup vs Seq'] = "1.00x"
    
    stats_df = pd.DataFrame(stats_data)
    st.dataframe(stats_df, use_container_width=True, hide_index=True)

except FileNotFoundError as e:
    st.error(f"❌ Arquivo não encontrado: {e}")
    st.info("""
    **Certifique-se de que os arquivos de FPS estão no diretório `benchmark/`:**
    - `fps_seq.txt`
    - `fps_omp.txt`
    - `fps_omp_static.txt`
    - `fps_cuda.txt`
    """)
except Exception as e:
    st.error(f"❌ Erro ao processar os dados: {e}")
    st.exception(e)
