import pandas as pd
import matplotlib.pyplot as plt

# Caminho do arquivo CSV
filename ="E:/data_log.csv"  # Altere se estiver usando nomes dinâmicos

# Lê o arquivo CSV
try:
    df = pd.read_csv(filename)
except FileNotFoundError:
    print(f"Arquivo '{filename}' não encontrado.")
    exit(1)

# Converte a coluna de tempo para segundos
df["time_sec"] = df["time"] / 1000.0  # tempo estava em ms

# Gráfico 1: Aceleração
plt.figure(figsize=(10, 5))
plt.title("Aceleração (eixos X, Y, Z)")
plt.plot(df["time_sec"], df["accel_x"], label="X", color='red')
plt.plot(df["time_sec"], df["accel_y"], label="Y", color='green')
plt.plot(df["time_sec"], df["accel_z"], label="Z", color='blue')
plt.xlabel("Tempo (s)")
plt.ylabel("Aceleração (g)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()

# Gráfico 2: Giroscópio
plt.figure(figsize=(10, 5))
plt.title("Velocidade Angular (Giroscópio) (eixos X, Y, Z)")
plt.plot(df["time_sec"], df["gyro_x"], label="X", color='red')
plt.plot(df["time_sec"], df["gyro_y"], label="Y", color='green')
plt.plot(df["time_sec"], df["gyro_z"], label="Z", color='blue')
plt.xlabel("Tempo (s)")
plt.ylabel("Velocidade Angular (°/s)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
