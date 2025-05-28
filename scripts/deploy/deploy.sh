#!/bin/bash

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Функция для вывода сообщений
log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Проверка наличия необходимых инструментов
check_requirements() {
    log "Checking requirements..."
    
    # Проверка Docker
    if ! command -v docker &> /dev/null; then
        error "Docker not found. Please install Docker."
        exit 1
    fi
    
    # Проверка kubectl
    if ! command -v kubectl &> /dev/null; then
        warn "kubectl not found. Kubernetes deployment will be skipped."
    fi
    
    # Проверка helm
    if ! command -v helm &> /dev/null; then
        warn "Helm not found. Helm charts deployment will be skipped."
    fi
}

# Создание Docker образа
build_docker_image() {
    log "Building Docker image..."
    docker build -t cloud-service:latest .
}

# Развертывание в Kubernetes
deploy_kubernetes() {
    if ! command -v kubectl &> /dev/null; then
        warn "Skipping Kubernetes deployment..."
        return
    fi
    
    log "Deploying to Kubernetes..."
    
    # Создание namespace
    kubectl create namespace cloud-service --dry-run=client -o yaml | kubectl apply -f -
    
    # Применение конфигурации
    kubectl apply -f k8s/configmap.yaml
    kubectl apply -f k8s/secrets.yaml
    kubectl apply -f k8s/deployment.yaml
    kubectl apply -f k8s/service.yaml
    
    # Ожидание готовности
    kubectl rollout status deployment/cloud-service -n cloud-service
}

# Развертывание с помощью Helm
deploy_helm() {
    if ! command -v helm &> /dev/null; then
        warn "Skipping Helm deployment..."
        return
    fi
    
    log "Deploying with Helm..."
    
    # Добавление репозитория
    helm repo add cloud-service https://charts.cloud-service.com
    helm repo update
    
    # Установка/обновление релиза
    helm upgrade --install cloud-service cloud-service/cloud-service \
        --namespace cloud-service \
        --create-namespace \
        --values helm/values.yaml
}

# Проверка развертывания
verify_deployment() {
    log "Verifying deployment..."
    
    # Проверка подов
    kubectl get pods -n cloud-service
    
    # Проверка сервисов
    kubectl get services -n cloud-service
    
    # Проверка логов
    kubectl logs -l app=cloud-service -n cloud-service
}

# Основная функция
main() {
    # Парсинг аргументов
    local use_kubernetes=false
    local use_helm=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --kubernetes)
                use_kubernetes=true
                shift
                ;;
            --helm)
                use_helm=true
                shift
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Выполнение шагов развертывания
    check_requirements
    build_docker_image
    
    if [ "$use_kubernetes" = true ]; then
        deploy_kubernetes
    fi
    
    if [ "$use_helm" = true ]; then
        deploy_helm
    fi
    
    verify_deployment
    
    log "Deployment completed successfully!"
}

# Запуск скрипта
main "$@"
