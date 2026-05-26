async function atualizarDashboard() {
    try {
        const response = await fetch('/api/status');
        const dados = await response.json();

        const slotsDiv = document.getElementById('slots-container');
        const usersDiv = document.getElementById('lista-usuarios');
        const tabela = document.getElementById('tabela-historico');

        const totalSlots = [1, 2, 3, 4];
        let slotsHtml = '';
        let usuariosHtml = '';

        totalSlots.forEach(id => {
            const slotData = dados.slots.find(s => s.id === id);
            const isOcupado = slotData && slotData.status === 'ocupado';
            const statusLabel = isOcupado ? 'OCUPADO' : 'LIVRE';

            slotsHtml += `
                <div class="slot-card">
                    <div style="font-size: 0.7rem; color: #666;">Slot ${id}</div>
                    <span class="status-badge ${isOcupado ? 'ocupado' : 'livre'}">${statusLabel}</span>
                </div>`;
            
            if(isOcupado && slotData.user_ra) {
                usuariosHtml += `<p style="margin: 10px 0;">${slotData.user_ra}</p>`;
            }
        });

        slotsDiv.innerHTML = slotsHtml;
        usersDiv.innerHTML = usuariosHtml || '<p style="color:#444">Nenhum kit fora</p>';

        tabela.innerHTML = dados.historico.map(h => {
            const data = new Date(h.data).toLocaleString('pt-BR');
            return `<tr><td>${data}</td><td>Nicho ${h.nicho_id}</td><td>${h.status}</td><td>${h.user_ra || '-'}</td></tr>`;
        }).join('');

    } catch (e) { console.error("Erro:", e); }
}

setInterval(atualizarDashboard, 3000);
atualizarDashboard();