# Documentation Doxygen — ControleurArrosage

## Prérequis

Les fichiers `.qml` ne sont pas nativement compris par Doxygen. La génération
utilise [doxyqml](https://github.com/agateau/doxyqml) pour les convertir en
pseudo-C++ avant analyse (propriétés, fonctions et signaux QML deviennent des
membres documentables).

Installation :

```bash
pip install doxyqml
```

`doxyqml` doit être accessible dans le PATH (le `Doxyfile` le référence via
`FILTER_PATTERNS = *.qml=doxyqml`).

## Génération

```bash
cd "Doxygen/documentation ControleurArrosage"
doxygen Doxyfile
```

La documentation HTML est générée directement dans `docs/ControleurArrosage/`
à la racine du dépôt (servie par GitHub Pages, voir `docs/index.html`).
