import os

project = "moba_sim"
author = "moba_sim developers"
version = release = "0.1.0"

extensions = ["myst_parser", "breathe"]

root_doc = "index"
exclude_patterns = ["build", "nix", "src", "tests", ".git", ".direnv"]

# Heading anchors follow GitHub's slug rules, so the same `#section-name`
# links in the prose work both on GitHub and in the generated site.
myst_heading_slug_func = "github"
myst_heading_anchors = 4
myst_heading_anchors_html_ids = True

html_theme = "furo"
html_title = "moba_sim"


breathe_projects = {"moba_sim": os.environ["MOBA_SIM_DOXYGEN_XML"]}
breathe_default_project = "moba_sim"
