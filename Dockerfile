# Базовый образ
FROM ubuntu:22.04

# Установка зависимостей
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libssl-dev \
    libnuma-dev \
    git \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Установка CUDA (опционально)
ARG ENABLE_CUDA=false
RUN if [ "$ENABLE_CUDA" = "true" ] ; then \
    wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin \
    && mv cuda-ubuntu2204.pin /etc/apt/preferences.d/cuda-repository-pin-600 \
    && wget https://developer.download.nvidia.com/compute/cuda/12.0.0/local_installers/cuda-repo-ubuntu2204-12-0-local_12.0.0-525.60.13-1_amd64.deb \
    && dpkg -i cuda-repo-ubuntu2204-12-0-local_12.0.0-525.60.13-1_amd64.deb \
    && apt-get update \
    && apt-get install -y cuda \
    && rm -rf /var/lib/apt/lists/* \
    ; fi

# Установка DPDK (опционально)
ARG ENABLE_DPDK=false
RUN if [ "$ENABLE_DPDK" = "true" ] ; then \
    apt-get update && apt-get install -y \
    libdpdk-dev \
    dpdk \
    && rm -rf /var/lib/apt/lists/* \
    ; fi

# Создание рабочей директории
WORKDIR /app

# Копирование исходного кода
COPY . .

# Сборка проекта
RUN mkdir build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_GPU=$ENABLE_CUDA \
        -DENABLE_DPDK=$ENABLE_DPDK \
    && make -j$(nproc) \
    && make install

# Создание пользователя
RUN useradd -m cloud-service

# Настройка прав доступа
RUN chown -R cloud-service:cloud-service /app

# Переключение на пользователя
USER cloud-service

# Настройка переменных окружения
ENV PATH="/usr/local/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"

# Открытие портов
EXPOSE 8080 9090 14268

# Запуск сервиса
CMD ["cloud-service"] 