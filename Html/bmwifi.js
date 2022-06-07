function setElementInnerText(elementID, innerText)
{
   let id = document.getElementById(elementID);
   if (id != null)
      id.innerText = innerText;
}


function getCookie(cName)
{
   const name = cName + "=";
   console.log (document.cookie);
   const cDecoded = decodeURIComponent(document.cookie);
   console.log (cDecoded);
   const cArr = cDecoded.split('; ');
   let res;
   cArr.forEach(val =>
   {
      if (val.indexOf(name) === 0) res = val.substring(name.length);
   })
   return res
}
