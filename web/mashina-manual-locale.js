/* SPDX-License-Identifier: GPL-3.0-only */
(() => {
 const lang=window.MashinaLocale?.code||"en";
 for(const article of document.querySelectorAll("[data-manual-lang]"))
   if(article.dataset.manualLang===lang) article.setAttribute("data-active","");
   else article.removeAttribute("data-active");
 const names={en:"User manual",es:"Manual de usuario",ja:"ユーザーマニュアル",ru:"Руководство пользователя",sr:"Корисничко упутство",hi:"उपयोगकर्ता मार्गदर्शिका"};
 document.title="MASHINA — "+(names[lang]||names.en);
 document.documentElement.lang=lang==="sr"?"sr-Cyrl":lang;
})();