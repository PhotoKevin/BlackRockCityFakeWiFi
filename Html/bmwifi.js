function setElementInnerText(elementID, innerText)
{
   let id = document.getElementById(elementID);
   if (id != null)
      id.innerText = innerText;
}

function setElementValue (elementID, value)
{
   let id = document.getElementById (elementID);
   if (id != null)
      id.value = value;
}


function getCookie (cName)
{
   const name = cName + "=";
   console.log (document.cookie);
   const cDecoded = decodeURIComponent (document.cookie);
   console.log (cDecoded);
   const cArr = cDecoded.split('; ');
   let res;
   cArr.forEach (val =>
   {
      if (val.indexOf(name) === 0) res = val.substring (name.length);
   })
   return res
}

function setTitle ()
{
   let title = getCookie ("title");
   if (title == null)
      title = "BRC Fake WiFi";
   document.title = title;
   setElementInnerText ("top", title);
}
