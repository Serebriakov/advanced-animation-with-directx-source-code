xof 0303txt 0032

template Path {
  <F8569BED-53B6-4923-AF0B-59A09271D556>
  DWORD Type;        // 0=straight, 1=curved
  Vector Start;
  Vector Point1;
  Vector Point2;
  Vector End;
}

template Route {
  <18AA1C92-16AB-47a3-B002-6178F9D2D12F>
  DWORD NumPaths;
  array Path Paths[NumPaths];
}

Route Robot {
  5;  // 5 paths

  0; // Straight path type
  0.0, 10.0,   0.0;  // Start
  0.0, 10.0,   0.0;  // Unused
  0.0, 10.0,   0.0;  // Unused
  0.0, 10.0, 150.0;, // End

  1; // Curved path type
    0.0, 10.0, 150.0;  // Start
   75.0, 10.0, 150.0;  // Point1
  150.0, 10.0,  75.0;  // Point2
  150.0, 10.0,   0.0;, // End

  1; // Curved path type
  150.0, 10.0,    0.0;  // Start
  150.0, 10.0,  -75.0;  // Point1
   75.0, 10.0, -150.0;  // Point2
    0.0, 10.0, -150.0;, // End

  0; // Straight path type
     0.0, 10.0, -150.0;  // Start
     0.0, 10.0,    0.0;  // Unused
     0.0, 10.0,    0.0;  // Unused
  -150.0, 10.0,   75.0;, // End

  0; // Straight path type
  -150.0, 10.0,  75.0;  // Start
     0.0, 10.0,   0.0;  // Unused
     0.0, 10.0,   0.0;  // Unused
     0.0, 10.0,   0.0;; // End
}

Route Camera {
  4;  // 4 paths

  1;  // Curved path type
    0.0, 80.0, 300.0;   // Start
  150.0, 80.0, 300.0;   // Point1
  300.0, 80.0, 150.0;   // Point2
  300.0, 80.0,   0.0;,  // End

  1;  // Curved path type
  300.0, 80.0,    0.0;   // Start
  300.0, 80.0, -150.0;   // Point1
  151.0, 80.0, -300.0;   // Point2
    0.0, 80.0, -300.0;,  // End

  1;  // Curved path type
     0.0, 80.0, -300.0;   // Start
  -150.0, 80.0, -300.0;   // Point1
  -300.0, 80.0, -150.0;   // Point2
  -300.0, 80.0,    0.0;,  // End

  1;  // Curved path type
  -300.0, 80.0,   0.0;   // Start
  -300.0, 80.0, 150.0;   // Point1
  -150.0, 80.0, 300.0;   // Point2
     0.0, 80.0, 300.0;;  // End
}

Route Target {
  5;  // 5 paths

  0; // Straight path type
  0.0, 10.0,   0.0;  // Start
  0.0, 10.0,   0.0;  // Unused
  0.0, 10.0,   0.0;  // Unused
  0.0, 10.0, 150.0;, // End

  1; // Curved path type
    0.0, 10.0, 150.0;  // Start
   75.0, 10.0, 150.0;  // Point1
  150.0, 10.0,  75.0;  // Point2
  150.0, 10.0,   0.0;, // End

  1; // Curved path type
  150.0, 10.0,    0.0;  // Start
  150.0, 10.0,  -75.0;  // Point1
   75.0, 10.0, -150.0;  // Point2
    0.0, 10.0, -150.0;, // End

  0; // Straight path type
     0.0, 10.0, -150.0;  // Start
     0.0, 10.0,    0.0;  // Unused
     0.0, 10.0,    0.0;  // Unused
  -150.0, 10.0,   75.0;, // End

  0; // Straight path type
  -150.0, 10.0,  75.0;  // Start
     0.0, 10.0,   0.0;  // Unused
     0.0, 10.0,   0.0;  // Unused
     0.0, 10.0,   0.0;; // End
}
