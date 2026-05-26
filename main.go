package main

import (
	"database/sql"
	"fmt"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	_ "github.com/glebarez/sqlite"
	tgbotapi "github.com/go-telegram-bot-api/telegram-bot-api/v5"
)

type Kit struct {
	NichoID int    `json:"nicho_id"`
	Status  string `json:"status"`
	UserRA  string `json:"user_ra"`
}

func main() {
	// 1. Conexão com o Banco
	db, err := sql.Open("sqlite", "./projeto.db")
	if err != nil {
		fmt.Println("Erro ao abrir banco:", err)
		return
	}
	defer db.Close()

	// 2. Criação da Tabela
	sqlStmt := `CREATE TABLE IF NOT EXISTS registros (
		id INTEGER PRIMARY KEY AUTOINCREMENT, 
		nicho_id INTEGER, 
		status TEXT, 
		user_ra TEXT, 
		created_at DATETIME DEFAULT CURRENT_TIMESTAMP
	);`
	_, err = db.Exec(sqlStmt)
	if err != nil {
		fmt.Println("Erro ao criar tabela:", err)
		return
	}

	// 3. Bot do Telegram 
go func() {
    bot, err := tgbotapi.NewBotAPI("8798262092:AAG-k2HFXiAg6e-jiOob6TJtY1g2-ixUdZ0")
    if err != nil {
        fmt.Println("Erro ao iniciar Bot:", err)
        return
    }

    u := tgbotapi.NewUpdate(0)
    u.Timeout = 60
    updates := bot.GetUpdatesChan(u)

    for update := range updates {
        if update.Message == nil || !update.Message.IsCommand() {
            continue
        }

        switch update.Message.Command() {
        
        case "limpar":
            _, err := db.Exec("DELETE FROM registros")
            if err != nil {
                bot.Send(tgbotapi.NewMessage(update.Message.Chat.ID, "❌ Erro ao limpar o banco."))
            } else {
                bot.Send(tgbotapi.NewMessage(update.Message.Chat.ID, "✅ Histórico limpo! Dashboard resetado."))
            }

        case "registro":
            rows, err := db.Query("SELECT nicho_id, status, user_ra, created_at FROM registros WHERE id IN (SELECT MAX(id) FROM registros GROUP BY nicho_id) ORDER BY nicho_id ASC")
            if err != nil {
                bot.Send(tgbotapi.NewMessage(update.Message.Chat.ID, "Erro ao buscar dados."))
                continue
            }

            var msgText string = "🤖 *Status Geral dos Kits*\n\n"
            horaConsulta := time.Now().Format("15:04")
            found := false

            for rows.Next() {
                var nID, status, userRA, timeStr string
                rows.Scan(&nID, &status, &userRA, &timeStr)
                found = true
                msgText += fmt.Sprintf("Kit %s | RA: %s | Status: %s (%s)\n", nID, userRA, status, horaConsulta)
            }
            rows.Close()

            if !found {
                msgText = "⚠️ Nenhum kit em uso no momento."
            }

            bot.Send(tgbotapi.NewMessage(update.Message.Chat.ID, msgText))
        }
    }
}()

	// 4. Servidor Web (Gin)
	r := gin.Default()

	r.Static("/static", "./templates")

	r.LoadHTMLGlob("templates/*.html")

	r.GET("/", func(c *gin.Context) {
    c.HTML(http.StatusOK, "index.html", nil)
})

	r.GET("/api/status", func(c *gin.Context) {
		rowsStatus, _ := db.Query("SELECT nicho_id, status, user_ra FROM registros WHERE id IN (SELECT MAX(id) FROM registros GROUP BY nicho_id)")
		defer rowsStatus.Close()

		var slots []gin.H
		for rowsStatus.Next() {
			var id int
			var st, ra string
			rowsStatus.Scan(&id, &st, &ra)
			slots = append(slots, gin.H{"id": id, "status": st, "user_ra": ra})
		}

		rowsHist, _ := db.Query("SELECT created_at, nicho_id, status, user_ra FROM registros ORDER BY id DESC LIMIT 10")
		defer rowsHist.Close()

		var historico []gin.H
		for rowsHist.Next() {
			var data, ra, st string
			var id int
			rowsHist.Scan(&data, &id, &st, &ra)
			historico = append(historico, gin.H{"data": data, "nicho_id": id, "status": st, "user_ra": ra})
		}

		c.JSON(http.StatusOK, gin.H{
			"slots":     slots,
			"historico": historico,
		})
	})

	r.POST("/kit/status", func(c *gin.Context) {
		var k Kit
		if err := c.ShouldBindJSON(&k); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "dados invalidos"})
			return
		}
		db.Exec("INSERT INTO registros (nicho_id, status, user_ra) VALUES (?, ?, ?)", k.NichoID, k.Status, k.UserRA)
		c.JSON(http.StatusOK, gin.H{"status": "ok"})
	})

	r.Run(":8080")
}