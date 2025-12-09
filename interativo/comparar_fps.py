import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import plotly.express as px
from pathlib import Path
import os

st.set_page_config(page_title="Comparação FPS - OpenMP vs CUDA", layout="wide")

st.title("📊 Comparação de Desempenho: OpenMP vs CUDA")
st.markdown("### Ray Tracer Interativo - Análise de Performance")

# Define o diretório correto
script_dir = Path(__file__).parent
fps_omp_path = script_dir / 'fps_omp.txt'
fps_cuda_path = script_dir / 'fps_cuda.txt'

# Tenta ler os arquivos
try:
    df_omp = pd.read_csv(fps_omp_path)
    df_cuda = pd.read_csv(fps_cuda_path)
    
    # Verifica se os arquivos existem e têm dados
    if df_omp.empty or df_cuda.empty:
        st.error("❌ Um ou ambos os arquivos estão vazios. Execute os programas primeiro!")
        st.stop()
    
    # Iguala o número de registros (pega o mínimo)
    min_records = min(len(df_omp), len(df_cuda))
    df_omp = df_omp.iloc[:min_records]
    df_cuda = df_cuda.iloc[:min_records]
    
    # Estatísticas gerais
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.metric(
            "FPS Médio - OpenMP",
            f"{df_omp['FPS'].mean():.2f}",
            delta=None
        )
    
    with col2:
        st.metric(
            "FPS Médio - CUDA",
            f"{df_cuda['FPS'].mean():.2f}",
            delta=f"{((df_cuda['FPS'].mean() / df_omp['FPS'].mean() - 1) * 100):.1f}%"
        )
    
    with col3:
        speedup = df_cuda['FPS'].mean() / df_omp['FPS'].mean()
        st.metric(
            "Speedup CUDA/OpenMP",
            f"{speedup:.2f}x",
            delta=None
        )
    
    st.markdown("---")
    
    # Gráfico de linha comparativo
    st.subheader("📈 FPS ao Longo do Tempo")
    
    fig = go.Figure()
    
    fig.add_trace(go.Scatter(
        x=df_omp['Tempo(s)'],
        y=df_omp['FPS'],
        mode='lines+markers',
        name='OpenMP',
        line=dict(color='#1f77b4', width=2),
        marker=dict(size=6)
    ))
    
    fig.add_trace(go.Scatter(
        x=df_cuda['Tempo(s)'],
        y=df_cuda['FPS'],
        mode='lines+markers',
        name='CUDA',
        line=dict(color='#ff7f0e', width=2),
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
    
    # Gráficos lado a lado
    col1, col2 = st.columns(2)
    
    with col1:
        st.subheader("📊 Distribuição de FPS - OpenMP")
        fig_hist_omp = px.histogram(
            df_omp,
            x='FPS',
            nbins=30,
            color_discrete_sequence=['#1f77b4']
        )
        fig_hist_omp.update_layout(
            xaxis_title="FPS",
            yaxis_title="Frequência",
            showlegend=False,
            height=350
        )
        st.plotly_chart(fig_hist_omp, use_container_width=True)
    
    with col2:
        st.subheader("📊 Distribuição de FPS - CUDA")
        fig_hist_cuda = px.histogram(
            df_cuda,
            x='FPS',
            nbins=30,
            color_discrete_sequence=['#ff7f0e']
        )
        fig_hist_cuda.update_layout(
            xaxis_title="FPS",
            yaxis_title="Frequência",
            showlegend=False,
            height=350
        )
        st.plotly_chart(fig_hist_cuda, use_container_width=True)

except FileNotFoundError as e:
    st.error(f"❌ Arquivo não encontrado: {e}")
    st.info("""
    **Como gerar os arquivos:**
    
    1. Execute a versão OpenMP:
       ```bash
       ./rayview_omp
       ```
    
    2. Execute a versão CUDA:
       ```bash
       ./rayview_cuda
       ```
    
    3. Execute este dashboard novamente.
    """)
except Exception as e:
    st.error(f"❌ Erro ao processar os dados: {e}")
    st.exception(e)

