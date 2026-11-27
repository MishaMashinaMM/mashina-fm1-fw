/* SPDX-License-Identifier: GPL-3.0-only
 * Shared MASHINA six-language navigation for all GitHub Pages tools.
 * Page-specific controls handle their own translations; this module
 * keeps user choice, page paths and the same two-line wordmark.
 */
(() => {
  "use strict";
  const langs = [["en","English"],["es","Español"],["ja","日本語"],["ru","Русский"],["sr","Српски"],["hi","हिन्दी"]];
  const qs = new URLSearchParams(location.search);
  // English is the canonical language of every bare page URL.
  // Only an explicit ?lang= selection changes the displayed translation.
  const requested = qs.get("lang");
  const code = langs.some(x => x[0] === requested) ? requested : "en";
  const choose = (next, refresh = false) => {
    if (!langs.some(x => x[0] === next)) return;
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
    if (!(/\/(emu|manual|webapp\/editor|webapp\/installer|web-library(?:\/guide\.html)?)\/?$/.test(u.pathname) ||
          u.pathname === "/mashina-fm1-fw/")) continue;
    u.searchParams.set("lang", code); a.href = u.href;
  }
  const labels = {
    en: {emu:"Emulator",installer:"Installer",editor:"Editor"},
    es: {emu:"Emulador",installer:"Instalador",editor:"Editor"},
    ja: {emu:"エミュレーター",installer:"インストーラー",editor:"エディター"},
    ru: {emu:"Эмулятор",installer:"Установщик",editor:"Редактор"},
    sr: {emu:"Емулатор",installer:"Инсталатер",editor:"Едитор"},
    hi: {emu:"एमुलेटर",installer:"इंस्टॉलर",editor:"एडिटर"}
  };
  for (const item of document.querySelectorAll("[data-nav-label]")) {
    const name = item.dataset.navLabel;
    item.textContent = (labels[code] || labels.en)[name] || item.textContent;
  }
  const header = document.querySelector(".mashina-brand");
  if (header) header.setAttribute("aria-label", "MASHINA — FM1 PERFORMANCE FW");
})();
