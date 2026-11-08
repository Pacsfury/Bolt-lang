const vscode = require("vscode");
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

const TASK_TYPE = "bolt";
const TASK_SOURCE = "Bolt";

function createCheckTask(file, scope, cwd, name) {
  const definition = { type: TASK_TYPE, file };
  const executable = findExecutable(cwd);
  const execution = new vscode.ProcessExecution(executable, ["check", file], { cwd });
  const task = new vscode.Task(
    definition,
    scope,
    name,
    TASK_SOURCE,
    execution,
    ["$bolt-check"]
  );
  task.group = vscode.TaskGroup.Build;

  return task;
}

function findExecutable(cwd) {
  let executable = vscode.workspace.getConfiguration("bolt").get("executable", "bolt");
  if (executable === "bolt") {
    const binary = process.platform === "win32" ? "bolt.exe" : "bolt";
    const candidates = [
      path.join(cwd, "cmake-build", "Release", binary),
      path.join(cwd, "cmake-build", "Debug", binary),
      path.join(cwd, "cmake-build", binary),
      path.join(cwd, "cmake-build-release", "Release", binary),
      path.join(cwd, "cmake-build-release", binary),
      path.join(cwd, binary)
    ];
    const localExecutable = candidates.find((candidate) => fs.existsSync(candidate));
    if (localExecutable) {
      executable = localExecutable;
    }
  }

  return executable;
}

function taskScope(workspaceFolder) {
  return workspaceFolder || vscode.TaskScope.Global;
}

function activate(context) {
  const diagnostics = vscode.languages.createDiagnosticCollection("bolt");
  const runningChecks = new Map();

  function checkDocument(document) {
    if (document.languageId !== "bolt" || document.uri.scheme !== "file") {
      return;
    }

    const uri = document.uri;
    const key = uri.toString();
    const previous = runningChecks.get(key);
    if (previous) {
      previous.kill();
    }

    diagnostics.delete(uri);
    const file = uri.fsPath;
    const cwd = vscode.workspace.getWorkspaceFolder(uri)?.uri.fsPath || path.dirname(file);
    const executable = findExecutable(cwd);
    const child = spawn(executable, ["check", file], { cwd, windowsHide: true });
    runningChecks.set(key, child);

    let output = "";
    let settled = false;
    const finish = (error) => {
      if (settled) {
        return;
      }
      settled = true;
      if (runningChecks.get(key) !== child) {
        return;
      }
      runningChecks.delete(key);

      const lines = output.split(/\r?\n/).filter((line) => line.trim().length > 0);
      if (error) {
        lines.unshift(error);
      }

      const entries = lines.map((line) => {
        const match = /^(.*\.blt): (error|warning): (.*)$/i.exec(line);
        const message = match ? match[3] : line;
        const severity = match && match[2].toLowerCase() === "warning"
          ? vscode.DiagnosticSeverity.Warning
          : vscode.DiagnosticSeverity.Error;
        const diagnostic = new vscode.Diagnostic(
          new vscode.Range(0, 0, 0, 1),
          message,
          severity
        );
        diagnostic.source = "Bolt";
        return diagnostic;
      });

      diagnostics.set(uri, entries);
    };

    child.stdout.on("data", (chunk) => {
      output += chunk.toString();
    });
    child.stderr.on("data", (chunk) => {
      output += chunk.toString();
    });
    child.on("error", (error) => {
      finish(`Could not run Bolt check: ${error.message}`);
    });
    child.on("close", (code, signal) => {
      if (code !== 0 && output.trim().length === 0) {
        finish(`Bolt check failed${signal ? ` (${signal})` : ` with exit code ${code}`}.`);
        return;
      }
      finish();
    });
  }

  const provider = {
    async provideTasks() {
      const files = await vscode.workspace.findFiles(
        "**/*.blt",
        "**/{.git,node_modules,build,cmake-build,cmake-build-release}/**"
      );

      return files.map((file) => {
        const workspaceFolder = vscode.workspace.getWorkspaceFolder(file);
        const cwd = workspaceFolder ? workspaceFolder.uri.fsPath : path.dirname(file.fsPath);
        const relativePath = workspaceFolder
          ? path.relative(workspaceFolder.uri.fsPath, file.fsPath)
          : path.basename(file.fsPath);
        const name = files.length === 1 ? "Bolt Check" : `Bolt Check: ${relativePath}`;

        return createCheckTask(file.fsPath, taskScope(workspaceFolder), cwd, name);
      });
    },

    resolveTask(task) {
      const file = task.definition.file;
      if (typeof file !== "string" || file.length === 0) {
        return undefined;
      }

      const workspaceFolder = task.scope && task.scope.uri ? task.scope : undefined;
      const cwd = workspaceFolder ? workspaceFolder.uri.fsPath : path.dirname(file);

      return createCheckTask(file, taskScope(workspaceFolder), cwd, task.name);
    }
  };

  context.subscriptions.push(
    diagnostics,
    vscode.tasks.registerTaskProvider(TASK_TYPE, provider),
    vscode.workspace.onDidOpenTextDocument(checkDocument),
    vscode.workspace.onDidSaveTextDocument((document) => {
      if (vscode.workspace.getConfiguration("bolt").get("checkOnSave", true)) {
        checkDocument(document);
      }
    }),
    vscode.workspace.onDidCloseTextDocument((document) => {
      const key = document.uri.toString();
      runningChecks.get(key)?.kill();
      runningChecks.delete(key);
      diagnostics.delete(document.uri);
    }),
    vscode.commands.registerCommand("bolt.check", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor || editor.document.languageId !== "bolt") {
        vscode.window.showErrorMessage("Open a Bolt source file before running Bolt Check.");
        return;
      }

      const workspaceFolder = vscode.workspace.getWorkspaceFolder(editor.document.uri);
      const cwd = workspaceFolder
        ? workspaceFolder.uri.fsPath
        : path.dirname(editor.document.uri.fsPath);
      const task = createCheckTask(
        editor.document.uri.fsPath,
        taskScope(workspaceFolder),
        cwd,
        "Bolt Check"
      );
      await vscode.tasks.executeTask(task);
    })
  );

  vscode.workspace.textDocuments.forEach(checkDocument);
}

function deactivate() {}

module.exports = { activate, deactivate };
