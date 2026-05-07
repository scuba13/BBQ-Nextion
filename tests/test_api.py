"""
Suite de integração — BBQ-Nextion REST API

Requer: pytest requests
Instalar: pip install pytest requests

Rodar:
    pytest tests/ --host 192.168.1.100 -v
    pytest tests/ --host 192.168.1.100 --key minha-chave -v
"""
import time
import pytest
import requests
from conftest import get_data


# ---------------------------------------------------------------------------
# 01 — Monitor
# ---------------------------------------------------------------------------

MONITOR_REQUIRED = {
    "bbqCurrentTemp", "bbqSetpoint",
    "proteinCurrentTemp", "proteinSetpoint",
    "relayState",
}

class TestMonitor:
    def test_returns_200(self, base_url, session):
        r = session.get(f"{base_url}/monitor")
        assert r.status_code == 200

    def test_campos_obrigatorios(self, base_url, session):
        r = session.get(f"{base_url}/monitor")
        data = get_data(r)
        ausentes = MONITOR_REQUIRED - data.keys()
        assert not ausentes, f"Campos ausentes no monitor: {ausentes}"

    def test_relay_state_valido(self, base_url, session):
        r = session.get(f"{base_url}/monitor")
        relay = get_data(r).get("relayState")
        assert relay in ("ON", "OFF"), f"relayState inválido: {relay!r}"

    def test_temperaturas_sao_numericas(self, base_url, session):
        data = get_data(session.get(f"{base_url}/monitor"))
        for campo in ("bbqCurrentTemp", "proteinCurrentTemp"):
            assert isinstance(data[campo], (int, float)), \
                f"{campo} deveria ser numérico, recebeu {type(data[campo])}"


# ---------------------------------------------------------------------------
# 02 — Configuração de setpoint (temperature/config)
# ---------------------------------------------------------------------------

class TestTemperatureConfig:
    def test_get_retorna_setpoints(self, base_url, session):
        r = session.get(f"{base_url}/temperature/config")
        assert r.status_code == 200
        data = get_data(r)
        assert "bbqTemperature" in data
        assert "proteinTemperature" in data

    def test_patch_valor_invalido_retorna_400(self, base_url, session):
        # Valor negativo — fora do intervalo mínimo configurado (padrão >= 30)
        r = session.patch(f"{base_url}/temperature/config",
                          data={"bbqTemperature": -999})
        assert r.status_code == 400, \
            f"Esperava 400 para valor inválido, recebeu {r.status_code}"

    def test_patch_valor_valido_persistido(self, base_url, session):
        TARGET = 120
        r = session.patch(f"{base_url}/temperature/config",
                          data={"bbqTemperature": TARGET})
        assert r.status_code == 200, f"PATCH falhou: {r.text}"
        # Confirma que o GET reflete o novo valor
        data = get_data(session.get(f"{base_url}/temperature/config"))
        assert data["bbqTemperature"] == TARGET, \
            f"Valor não persistido: esperava {TARGET}, recebeu {data['bbqTemperature']}"

    def test_patch_proteina_valor_valido(self, base_url, session):
        TARGET = 65
        r = session.patch(f"{base_url}/temperature/config",
                          data={"proteinTemperature": TARGET})
        assert r.status_code == 200
        data = get_data(session.get(f"{base_url}/temperature/config"))
        assert data["proteinTemperature"] == TARGET


# ---------------------------------------------------------------------------
# 03 — Limites de temperatura (temp/config)
# ---------------------------------------------------------------------------

class TestTempLimits:
    CAMPOS = ["minBBQTemp", "maxBBQTemp", "minPrtTemp", "maxPrtTemp",
              "minCaliTemp", "maxCaliTemp"]

    def test_get_retorna_limites(self, base_url, session):
        r = session.get(f"{base_url}/temp/config")
        assert r.status_code == 200
        data = get_data(r)
        ausentes = set(self.CAMPOS) - data.keys()
        assert not ausentes, f"Limites ausentes: {ausentes}"

    def test_limites_sao_inteiros(self, base_url, session):
        data = get_data(session.get(f"{base_url}/temp/config"))
        for campo in self.CAMPOS:
            assert isinstance(data[campo], int), \
                f"{campo} deveria ser int, recebeu {type(data[campo])}"

    def test_min_menor_que_max(self, base_url, session):
        data = get_data(session.get(f"{base_url}/temp/config"))
        assert data["minBBQTemp"] < data["maxBBQTemp"]
        assert data["minPrtTemp"] < data["maxPrtTemp"]


# ---------------------------------------------------------------------------
# 04 — Diagnósticos
# ---------------------------------------------------------------------------

DIAG_SECOES = {"heap", "cpu", "system", "wifi", "mqtt", "sensors", "tasks", "relay"}

class TestDiagnostics:
    def test_retorna_200(self, base_url, session):
        r = session.get(f"{base_url}/diagnostics")
        assert r.status_code == 200

    def test_secoes_obrigatorias(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        ausentes = DIAG_SECOES - data.keys()
        assert not ausentes, f"Seções ausentes: {ausentes}"

    def test_sistema_saudavel(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        assert data["system"]["healthy"] is True, \
            "Sistema reportou não saudável — verificar heap/stack/fragmentation"

    def test_heap_free_positivo(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        assert data["heap"]["free"] > 0

    def test_wifi_conectado(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        assert data["wifi"]["connected"] is True, \
            "WiFi reportou desconectado — teste via HTTP só funciona com WiFi ativo"

    def test_reset_reason_presente(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        assert isinstance(data["system"].get("resetReason"), str)
        assert len(data["system"]["resetReason"]) > 0

    def test_tasks_stacks_positivos(self, base_url, session):
        data = get_data(session.get(f"{base_url}/diagnostics"))
        tasks = data["tasks"]
        assert tasks["tempStack"] > 0, "TempTask stack = 0 (task não está rodando?)"
        assert tasks["controlStack"] > 0, "ControlTask stack = 0 (task não está rodando?)"


# ---------------------------------------------------------------------------
# 05 — Log
# ---------------------------------------------------------------------------

class TestLog:
    def test_get_log_retorna_200(self, base_url, session):
        r = session.get(f"{base_url}/log/content")
        assert r.status_code == 200

    def test_log_nao_vazio(self, base_url, session):
        r = session.get(f"{base_url}/log/content")
        # O log pode estar no campo data ou direto no texto
        body = r.json()
        content = body.get("data", {})
        if isinstance(content, dict):
            log_text = content.get("log", content.get("content", ""))
        else:
            log_text = str(content)
        assert len(log_text) > 0, "Log está vazio — esperava pelo menos a entrada de BOOT"


# ---------------------------------------------------------------------------
# 06 — Injeção de debug (Q-15)
# ---------------------------------------------------------------------------

class TestDebugInject:
    def test_inject_reflete_no_monitor(self, base_url, session):
        BBQ = 175
        PRT = 68

        r = session.post(f"{base_url}/debug/inject-temp",
                         data={"bbqTemp": BBQ, "proteinTemp": PRT, "seconds": 10})
        assert r.status_code == 200, f"Inject falhou: {r.text}"

        monitor = get_data(session.get(f"{base_url}/monitor"))
        assert monitor["bbqCurrentTemp"] == BBQ, \
            f"Monitor não refletiu bbqTemp injetado: {monitor['bbqCurrentTemp']}"
        assert monitor["proteinCurrentTemp"] == PRT, \
            f"Monitor não refletiu proteinTemp injetado: {monitor['proteinCurrentTemp']}"

        # Cancela a injeção antes de sair
        session.delete(f"{base_url}/debug/inject-temp")

    def test_inject_parametros_invalidos_retorna_400(self, base_url, session):
        r = session.post(f"{base_url}/debug/inject-temp",
                         data={"bbqTemp": -50, "proteinTemp": 600, "seconds": 5})
        assert r.status_code == 400

    def test_inject_seconds_invalido_retorna_400(self, base_url, session):
        r = session.post(f"{base_url}/debug/inject-temp",
                         data={"bbqTemp": 100, "proteinTemp": 60, "seconds": 9999})
        assert r.status_code == 400

    def test_delete_cancela_injecao(self, base_url, session):
        session.post(f"{base_url}/debug/inject-temp",
                     data={"bbqTemp": 200, "proteinTemp": 90, "seconds": 60})
        r = session.delete(f"{base_url}/debug/inject-temp")
        assert r.status_code == 200


# ---------------------------------------------------------------------------
# 07 — Autenticação (cuidado: limpa chave ao final)
# ---------------------------------------------------------------------------

class TestAuth:
    def test_get_auth_config(self, base_url, session):
        r = session.get(f"{base_url}/auth/config")
        assert r.status_code == 200
        data = get_data(r)
        assert "keyConfigured" in data

    def test_auth_flow_completo(self, base_url, api_key):
        """
        Testa set → uso sem chave → uso com chave → remoção.
        Pulado se já houver chave configurada (--key foi passado).
        """
        if api_key:
            pytest.skip("Chave já configurada via --key; pulando para não sobrescrever")

        data = get_data(requests.get(f"{base_url}/auth/config", timeout=5))
        if data.get("keyConfigured"):
            pytest.skip("Dispositivo já tem chave configurada; pulando teste de auth")

        TEST_KEY = "bbq-test-key-2026"

        # 1. Define a chave
        r = requests.patch(f"{base_url}/auth/config",
                           data={"newKey": TEST_KEY}, timeout=5)
        assert r.status_code == 200, f"Falha ao definir chave: {r.text}"

        # 2. Escrita sem chave deve falhar
        r = requests.patch(f"{base_url}/auth/config",
                           data={"newKey": "outra"}, timeout=5)
        assert r.status_code == 401, \
            f"Esperava 401 sem chave, recebeu {r.status_code}"

        # 3. Leitura sem chave deve funcionar
        r = requests.get(f"{base_url}/auth/config", timeout=5)
        assert r.status_code == 200

        # 4. Remove a chave (cleanup)
        r = requests.patch(f"{base_url}/auth/config",
                           data={"currentKey": TEST_KEY, "newKey": ""},
                           headers={"X-API-Key": TEST_KEY}, timeout=5)
        assert r.status_code == 200, f"Falha ao remover chave: {r.text}"

        # 5. Confirma remoção
        data = get_data(requests.get(f"{base_url}/auth/config", timeout=5))
        assert not data.get("keyConfigured"), "Chave ainda aparece como configurada após remoção"


# ---------------------------------------------------------------------------
# 08 — System reset
# ---------------------------------------------------------------------------

class TestSystemReset:
    def test_reset_retorna_200(self, base_url, session):
        r = session.post(f"{base_url}/system/reset")
        assert r.status_code == 200

    def test_apos_reset_setpoints_zerados(self, base_url, session):
        session.post(f"{base_url}/system/reset")
        time.sleep(0.5)  # pequeno delay para o reset processar
        data = get_data(session.get(f"{base_url}/temperature/config"))
        assert data["bbqTemperature"] == 0, \
            f"bbqTemperature não zerou após reset: {data['bbqTemperature']}"
        assert data["proteinTemperature"] == 0, \
            f"proteinTemperature não zerou após reset: {data['proteinTemperature']}"
