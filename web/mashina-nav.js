/* SPDX-License-Identifier: GPL-3.0-only
 * Shared MASHINA six-language navigation for all GitHub Pages tools.
 * Page-specific controls handle their own translations; this module
 * keeps user choice, page paths and the same two-line wordmark.
 */
(() => {
  "use strict";
  const langs = [["en","English"],["es","Español"],["ja","日本語"],["ru","Русский"],["sr","Српски"],["hi","हिन्दी"]];
  const qs = new URLSearchParams(location.search);
  let code = qs.get("lang");
  if (!langs.some(x => x[0] === code)) {
    try { code = localStorage.getItem("mashina-lang"); } catch (_) {}
  }
  if (!langs.some(x => x[0] === code)) {
    const browser = (navigator.language || "en").toLowerCase();
    code = browser.startsWith("sr") ? "sr" : browser.startsWith("hi") ? "hi" : langs.find(x => browser.startsWith(x[0]))?.[0] || "en";
  }
  const choose = (next, refresh = false) => {
    if (!langs.some(x => x[0] === next)) return;
    try { localStorage.setItem("mashina-lang", next); } catch (_) {}
    const u = new URL(location.href); u.searchParams.set("lang", next);
    if (refresh) location.assign(u.href);
    else history.replaceState(null, "", u.pathname + u.search + u.hash);
  };
  window.MashinaLocale = { code, choose, langs };
  const nav = document.getElementById("mashina-langs");
  if (nav) {
    nav.setAttribute("aria-label", "Language / Језик / भाषा");
    nav.classList.add("mashina-langs");
    for (const [id, name] of langs) {
      const button = document.createElement("button");
      button.type = "button";
      button.lang = id === "sr" ? "sr-Cyrl" : id;
      button.textContent = name;
      button.dataset.lang = id;
      button.setAttribute("aria-pressed", String(id === code));
      button.addEventListener("click", () => choose(id, true));
      nav.append(button);
    }
  }
  for (const a of document.querySelectorAll("a[href]")) {
    if (a.closest(".mashina-langs")) continue;
    const u = new URL(a.href, location.href);
    if (u.origin !== location.origin || !u.pathname.startsWith("/mashina-fm1-fw/")) continue;
    if (!(/\/(emu|manual|webapp\/editor|webapp\/installer)\/?$/.test(u.pathname) ||
          u.pathname === "/mashina-fm1-fw/")) continue;
    u.searchParams.set("lang", code); a.href = u.href;
  }
  const header = document.querySelector(".mashina-brand");
  if (header) header.setAttribute("aria-label", "MASHINA — FM1 PERFORMANCE FW");
})();
