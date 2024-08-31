package testing

import (
	"context"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/jackc/pgx/v4"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type EmbeddedPG struct {
	Host        string
	Port        int
	PostgresDir string
}

func (e *EmbeddedPG) CreateUser(username, password string) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	connString := fmt.Sprintf("host=%s port=%d user=%s", e.Host, e.Port, "postgres")
	connConfig, err := pgx.ParseConfig(connString)
	if err != nil {
		return err
	}
	connConfig.BuildStatementCache = nil
	connConfig.PreferSimpleProtocol = true

	conn, err := pgx.ConnectConfig(ctx, connConfig)
	if err != nil {
		return xerrors.Errorf("can't connect to database: %w", err)
	}
	defer func() {
		_ = conn.Close(ctx)
	}()

	_, err = conn.Exec(ctx, fmt.Sprintf("CREATE USER %s WITH PASSWORD '%s'", username, password))
	if err != nil {
		return xerrors.Errorf("can't create user: %w", err)
	}

	return nil
}

func (e *EmbeddedPG) CreateDatabase(dbname string) error {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	connString := fmt.Sprintf("host=%s port=%d user=%s", e.Host, e.Port, "postgres")
	connConfig, err := pgx.ParseConfig(connString)
	if err != nil {
		return err
	}
	connConfig.BuildStatementCache = nil
	connConfig.PreferSimpleProtocol = true

	conn, err := pgx.ConnectConfig(ctx, connConfig)
	if err != nil {
		return xerrors.Errorf("can't connect to database: %w", err)
	}
	defer func() {
		_ = conn.Close(ctx)
	}()

	_, err = conn.Exec(ctx, fmt.Sprintf("CREATE DATABASE %s", dbname))
	if err != nil {
		return xerrors.Errorf("can't create database: %w", err)
	}

	return nil
}

func (e *EmbeddedPG) ApplyMigrations(migrationsFolder, dbname string) error {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	connString := fmt.Sprintf("host=%s port=%d user=%s dbname=%s", e.Host, e.Port, "postgres", dbname)
	connConfig, err := pgx.ParseConfig(connString)
	if err != nil {
		return err
	}
	connConfig.BuildStatementCache = nil
	connConfig.PreferSimpleProtocol = true

	conn, err := pgx.ConnectConfig(ctx, connConfig)
	if err != nil {
		return xerrors.Errorf("can't connect to database: %w", err)
	}
	defer func() {
		_ = conn.Close(ctx)
	}()

	migrationsDir := filepath.Join(migrationsFolder, "migrations")
	grantsDir := filepath.Join(migrationsFolder, "grants")

	for _, dir := range []string{migrationsDir, grantsDir} {
		files, err := ioutil.ReadDir(dir) // sorting by name is important here
		if err != nil {
			return xerrors.Errorf("failed to find migrations dir: %w", err)
		}

		for _, file := range files {
			sql, err := ioutil.ReadFile(filepath.Join(dir, file.Name()))
			if err != nil {
				return xerrors.Errorf("failed to read migration file: %w", err)
			}

			_, err = conn.Exec(ctx, string(sql))
			if err != nil {
				return xerrors.Errorf("failed to apply migration: %w", err)
			}
		}
	}

	return nil
}

func (e *EmbeddedPG) StartPostgres() error {
	postgresDir, err := preparePostgresBinaries()
	if err != nil {
		return err
	}
	e.PostgresDir = postgresDir

	dataDir := filepath.Join(postgresDir, "pg_data")
	if _, err := os.Stat(dataDir); !os.IsNotExist(err) {
		if err := os.RemoveAll(dataDir); err != nil {
			return err
		}
	}
	if err := os.Mkdir(dataDir, os.ModePerm); err != nil {
		return err
	}

	initdbBin := filepath.Join(postgresDir, "bin/initdb")
	initCmd := exec.Command(initdbBin, "--nosync", "-A", "trust", "-U", "postgres", "-D", dataDir, "--no-locale", "-E", "UTF-8")
	if err := initCmd.Run(); err != nil {
		return err
	}

	pgCtlBin := filepath.Join(postgresDir, "bin/pg_ctl")
	startCmd := exec.Command(pgCtlBin, "-D", dataDir, "-o", fmt.Sprintf("-p%d", e.Port), "-o", "-F", "start")
	if err := startCmd.Run(); err != nil {
		return err
	}

	return nil
}

func (e *EmbeddedPG) StopPostgres() error {
	dataDir := filepath.Join(e.PostgresDir, "pg_data")

	pgCtlBin := filepath.Join(e.PostgresDir, "bin/pg_ctl")
	stopCmd := exec.Command(pgCtlBin, "-D", dataDir, "stop", "-m", "immediate", "-t", "5")
	if err := stopCmd.Run(); err != nil {
		return err
	}

	return nil
}

func getSandboxResourceID() (string, error) {
	switch runtime.GOOS {
	case "windows":
		return "", xerrors.New("No binary for windows uploaded, see instruction: <TODO>")
	case "darwin":
		return "1580325269", nil // https://sandbox.yandex-team.ru/resource/1580325269/view
	case "linux":
		return "1580624236", nil // https://sandbox.yandex-team.ru/resource/1580624236/view
	default:
		return "", xerrors.New("Unsupported OS type")
	}
}

func downloadSandboxResource(path string, resource string) error {
	downloadURL := fmt.Sprintf("https://proxy.sandbox.yandex-team.ru/%s", resource)
	request := resty.New().SetRetryCount(3).SetDoNotParseResponse(true).R()
	res, err := request.Get(downloadURL)
	if err != nil {
		return err
	}

	body := res.RawBody()
	defer body.Close()

	out, err := os.Create(path)
	if err != nil {
		return err
	}
	defer out.Close()

	_, err = io.Copy(out, body)
	return err
}

func extractArchive(archive, destination string) error {
	return Unarchive(archive, destination)
}

func preparePostgresBinaries() (string, error) {
	sandboxID, err := getSandboxResourceID()
	if err != nil {
		return "", err
	}

	pgDir := filepath.Join(os.TempDir(), fmt.Sprintf("embedded-pg-%s", sandboxID))
	fmt.Printf("embedded postgres path: %s\n", pgDir)

	if _, err := os.Stat(pgDir); os.IsNotExist(err) {
		if err := os.MkdirAll(pgDir, os.ModePerm); err != nil {
			return "", err
		}

		archive := filepath.Join(pgDir, "postgresql.zip")
		if err := downloadSandboxResource(archive, sandboxID); err != nil {
			return "", err
		}
		if err := extractArchive(archive, pgDir); err != nil {
			return "", err
		}
	}

	return pgDir, nil
}
