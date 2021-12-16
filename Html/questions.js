{
   var answered = false;
   function answerClick (nextPage, commentary)
   {
      console.log (nextPage);
      console.log (commentary);
      if (answered)
      {
         window.location.href = nextPage;
      }
      else
      {
         answered = true;
         var correct = 0;
         answerButton.innerHTML = "Next";
         const inputs = document.getElementsByTagName ("input");
         if (inputs != null)
         {
            for (var i=0; i<inputs.length; i++)
            {
               inputs[i].disabled = true;
               if (inputs[i].dataset.correct == "true")
               {
                  if (inputs[i].checked)
                     correct = 1;
                  inputs[i].parentNode.style.opacity = "1.0";
               }
               else
                  inputs[i].parentNode.style.opacity = "0.1";
            }
         }

         if (correct)
            solution.innerHTML = "Correct";
         else
            solution.innerHTML = "Sorry";

         if (commentary != "")
            solution.innerHTML += " - " + commentary;
      }
   }

   function selectionChanged ()
   {
      answerButton.disabled = false;
   }

   // Randomize array in-place using Durstenfeld shuffle algorithm
   function shuffleArray (array)
   {
      for (var i = array.length - 1; i > 0; i--)
      {
         var j = Math.floor(Math.random() * (i + 1));
         var temp = array[i];
         array[i] = array[j];
         array[j] = temp;
      }
   }
}
