{
   var questionSet;
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


   var nextQuestionPage;

   function getQuestionNumber ()
   {
      var qn = 0;
      const queryString = window.location.search;
      const urlParams = new URLSearchParams (window.location.search);
      if (urlParams.get ('q') != null)
         qn = Number (urlParams.get ('q'));

      if (isNaN (qn) || (qn < 0) || (questionSet == null) || (qn > (questionSet.length-1)))
         qn = 0;

      return qn
   }

   function getNextPage ()
   {
      let nextNumber = 1 + getQuestionNumber ();
      let nextQuestionPage = "question.html?q="+nextNumber;

      if (nextNumber >= questionSet.length)
         nextQuestionPage = "blocked.html";

      return nextQuestionPage;
   }

   const xhr = new XMLHttpRequest();
   function ajaxHandler ()
   {
      if (xhr.readyState === XMLHttpRequest.DONE)
      {
         if (xhr.status === 200)
         {
            var data = JSON.parse (xhr.responseText);
            questionSet = data;
            displayQuestion (getQuestionNumber());
         }
         else
         {
            //alert('There was a problem with the request: ' + xhr.status);
         }
      }
   }

   function getQuestionSet (qNumber)
   {
      displayQuestion (getQuestionNumber());

      if (window.location.protocol == "file:")
      {
         displayQuestion (getQuestionNumber());
      }
      else
      {
         const url = 'getJson';
         var formData = new FormData();
         formData.append ("request", "question");

         xhr.open ('POST', url, true);
         xhr.onreadystatechange = ajaxHandler;
         xhr.send (formData);
      }
   }


   function displayQuestion (qn)
   {
      let questionData = questionSet[qn];

      answers = questionData.answers;
      commentary = questionData.commentary;
      questionDiv.innerText = questionData.question;

      for (let i=0; i<10; i++)
      {
         let optdiv = document.getElementById ("optdiv" + i);
         if (optdiv != null)
            optdiv.hidden = true;
      }

      for (let i=0; i<answers.length; i++)
      {
         let optdiv = document.getElementById ("optdiv" + i);
         let optlabel = document.getElementById ("optlabel" + i);
         let optinput = document.getElementById ("optinput" + i);
         if (optdiv != null)
         {
            optdiv.hidden = false;
            optlabel.innerText = answers[i].text;
            optinput.dataset.correct = answers[i].correct;
//            if (optinput.dataset.correct == "true")
//               optdiv.style.color = "blue";
         }
      }
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
